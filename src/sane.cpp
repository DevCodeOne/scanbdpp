#include "common.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <regex>
#include <thread>

#include "signal.h"

#include "sane.h"
#include "sanepp.h"

#include "config.h"

namespace scanbdpp {

    void SaneHandler::start() {
        if (_device_threads.size() != 0) {
            stop();
        }

        // critical section
        {
            sanepp::Sane sane_instance;
            std::lock_guard<std::mutex> device_guard(_device_mutex);
            for (auto device_info : sane_instance.devices(false)) {
                _device_threads.emplace_back(std::make_unique<PollHandler>(sane_instance, device_info));
            }
        }
    }

    void SaneHandler::stop() {
        if (_device_threads.size() == 0) {
            return;
        }

        // critical section
        {
            std::lock_guard<std::mutex> device_guard(_device_mutex);
            for (auto &current_handler : _device_threads) {
                // stopping poll thread for device
                current_handler->stop();

                if (current_handler->poll_thread().joinable()) {
                    current_handler->poll_thread().join();
                }
            }

            _device_threads.clear();
        }
    }

    void SaneHandler::start_device_polling(const std::string &device_name) {
        std::lock_guard<std::mutex> device_guard(_device_mutex);

        auto thread_with_device = [&device_name](const auto &current_handler) {
            return current_handler->device_info().name() == device_name;
        };

        if (std::any_of(_device_threads.cbegin(), _device_threads.cend(), thread_with_device)) {
            return;
        }

        sanepp::Sane sane_instance;
        auto devices = sane_instance.devices(false);

        auto device_info = std::find_if(devices.cbegin(), devices.cend(), [&device_name](const auto &current_info) {
            return current_info.name() == device_name;
        });

        if (device_info != devices.cend()) {
            _device_threads.emplace_back(std::make_unique<PollHandler>(sane_instance, *device_info));
        }
    }

    void SaneHandler::stop_device_polling(const std::string &device_name) {
        std::lock_guard<std::mutex> device_guard(_device_mutex);

        auto thread_with_device = [&device_name](const auto &current_handler) {
            return current_handler->device_info().name() == device_name;
        };

        auto device = std::find_if(_device_threads.cbegin(), _device_threads.cend(), thread_with_device);

        if (device == _device_threads.cend()) {
            return;
        }

        auto &poll_handler = *(device->get());

        poll_handler.stop();

        if (poll_handler.poll_thread().joinable()) {
            poll_handler.poll_thread().join();
        }

        _device_threads.erase(std::remove_if(_device_threads.begin(), _device_threads.end(),
                                             [&device_name](const auto &current_handler) {
                                                 return current_handler->device_info().name() == device_name;
                                             }),
                              _device_threads.end());
    }

    SaneHandler::PollHandler::PollHandler(sanepp::Sane instance, sanepp::DeviceInfo device_info)
        : m_instance(instance),
          m_device_info(device_info),
          m_terminate(false),
          m_poll_thread(std::bind(&SaneHandler::PollHandler::poll_device, this)) {}

    SaneHandler::PollHandler::PollHandler(PollHandler &&handler)
        : m_instance(handler.m_instance),
          m_device_info(std::move(handler.m_device_info)),
          m_terminate((bool)handler.m_terminate),
          m_poll_thread(std::move(handler.m_poll_thread)) {}

    void SaneHandler::PollHandler::stop() { m_terminate = true; }

    const sanepp::DeviceInfo &SaneHandler::PollHandler::device_info() const { return m_device_info; }

    const std::atomic_bool &SaneHandler::PollHandler::should_stop() const { return m_terminate; }

    const std::thread &SaneHandler::PollHandler::poll_thread() const { return m_poll_thread; }

    std::thread &SaneHandler::PollHandler::poll_thread() { return m_poll_thread; }

    // TODO change placeholder strings
    void SaneHandler::PollHandler::find_matching_functions(const sanepp::Device &device,
                                                           const confusepp::Section &root) {
        Config config;

        if (auto function_multi_section = root.get<confusepp::Multisection>("functions"); function_multi_section) {
            auto function_sections = function_multi_section->sections();

            for (auto current_function : function_sections) {
                if (auto filter = current_function.get<confusepp::Option<std::string>>("filter"); filter) {
                    std::regex function_regex;
                    bool regex_is_valid = true;
                    try {
                        function_regex.assign(filter->value(), std::regex_constants::extended);
                    } catch (std::regex_error) {
                        // Couldn't compile regex
                        regex_is_valid = false;
                    }

                    if (!regex_is_valid) {
                        continue;
                    }

                    for (auto current_option : device.options()) {
                        if (!std::regex_match(current_option.info().name(), function_regex)) {
                            continue;
                        }

                        if (auto env = current_function.get<confusepp::Option<std::string>>("env"); env) {
                            auto function_with_env =
                                std::find_if(m_functions.begin(), m_functions.end(),
                                             [&env](const Function &current) { return current.m_env == env->value(); });
                            if (function_with_env != m_functions.cend()) {
                                // Warning for overriding function
                                function_with_env->m_option = current_option;
                            } else {
                                m_functions.emplace_back(Function{current_option, env->value()});
                            }
                        } else {
                            // Error no env is set
                        }
                    }
                }
            }
        }
    }

    // TODO change placeholder strings
    void SaneHandler::PollHandler::find_matching_options(const sanepp::Device &device, const confusepp::Section &root) {
        Config config;

        if (auto action_multi_section = root.get<confusepp::Multisection>("actions"); action_multi_section) {
            auto action_sections = action_multi_section->sections();

            for (auto current_action : action_sections) {
                if (auto filter = current_action.get<confusepp::Option<std::string>>("filter"); filter) {
                    std::regex action_regex;
                    bool regex_is_valid = true;
                    try {
                        action_regex.assign(filter->value(), std::regex_constants::extended);
                    } catch (std::regex_error) {
                        // Couldn't compile regex
                        regex_is_valid = false;
                    }

                    if (!regex_is_valid) {
                        continue;
                    }

                    auto script = current_action.get<confusepp::Option<std::string>>("script");

                    // TODO discuss if it should be possible to install an action with a script to execute
                    if (!script) {
                        // Error no script set
                        continue;
                    }

                    for (auto current_option : device.options()) {
                        if (!std::regex_match(current_option.info().name(), action_regex)) {
                            continue;
                        }

                        auto multiple_actions_allowed = config.get<confusepp::Option<bool>>("/global/multiple_actions");

                        auto option_with_script = std::find_if(
                            m_actions.begin(), m_actions.end(),
                            [&current_option](const auto &action) { return action.m_option == current_option; });

                        if (option_with_script != m_actions.cend() && !multiple_actions_allowed) {
                            // overwriting existing action
                            option_with_script->m_option = current_option;
                        } else {
                            // adding additional action
                            m_actions.emplace_back(current_option);
                            option_with_script = m_actions.end() - 1;
                        }

                        option_with_script->m_action_name = current_action.title();
                        option_with_script->m_script = script->value();
                    }
                }
            }
        }
    }

    void SaneHandler::PollHandler::poll_device() {
        // Disable all signal handling in this thread
        sigset_t mask;
        sigfillset(&mask);
        pthread_sigmask(SIG_BLOCK, &mask, nullptr);

        auto device = device_info().open();

        if (!device) {
            // Error couldn't open device
            return;
        }

        while (!should_stop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

}  // namespace scanbdpp
