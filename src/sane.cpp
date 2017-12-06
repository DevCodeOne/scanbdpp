#include "common.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <regex>
#include <thread>

#include <signal.h>

#include <spdlog/spdlog.h>

#include "sane.h"
#include "sanepp.h"
#include "signal_handler.h"

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

    void SaneHandler::PollHandler::find_matching_functions(const sanepp::Device &device,
                                                           const confusepp::Section &root) {
        Config config;

        if (auto function_multi_section = root.get<confusepp::Multisection>(Config::Constants::function);
            function_multi_section) {
            for (auto current_function : function_multi_section->sections()) {
                if (auto filter = current_function.get<confusepp::Option<std::string>>(Config::Constants::filter);
                    filter) {
                    std::regex function_regex;
                    bool regex_is_valid = true;
                    try {
                        function_regex.assign(filter->value(), std::regex_constants::extended);
                    } catch (std::regex_error) {
                        spdlog::get("logger")->critical("Couldn't compile regex");
                        regex_is_valid = false;
                    }

                    if (!regex_is_valid) {
                        continue;
                    }

                    for (auto current_option : device.options()) {
                        if (!std::regex_match(current_option.info().name(), function_regex)) {
                            continue;
                        }

                        if (auto env = current_function.get<confusepp::Option<std::string>>(Config::Constants::env);
                            env) {
                            auto function_with_env =
                                std::find_if(m_functions.begin(), m_functions.end(),
                                             [&env](const Function &current) { return current.m_env == env->value(); });
                            if (function_with_env != m_functions.cend()) {
                                // Warning for overriding function
                                function_with_env->m_option_info = current_option.info();
                            } else {
                                m_functions.emplace_back(Function{current_option.info(), env->value()});
                            }
                        } else {
                            // Error no env is set
                        }
                    }
                }
            }
        }
    }

    void SaneHandler::PollHandler::find_matching_options(const sanepp::Device &device, const confusepp::Section &root) {
        Config config;

        if (auto action_multi_section = root.get<confusepp::Multisection>(Config::Constants::action);
            action_multi_section) {
            auto action_sections = action_multi_section->sections();

            for (auto current_action : action_sections) {
                if (auto filter = current_action.get<confusepp::Option<std::string>>(Config::Constants::filter);
                    filter) {
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

                    auto script = current_action.get<confusepp::Option<std::string>>(Config::Constants::script);

                    // TODO discuss if it should be possible to install an action without a script to execute
                    if (!script) {
                        // Error no script set
                        continue;
                    }

                    for (auto current_option : device.options()) {
                        if (!std::regex_match(current_option.info().name(), action_regex)) {
                            continue;
                        }

                        auto multiple_actions_allowed = config.get<confusepp::Option<bool>>(
                            Config::Constants::global / Config::Constants::multiple_actions);

                        auto option_with_script =
                            std::find_if(m_actions.begin(), m_actions.end(), [&current_option](const auto &action) {
                                return action.m_option_info == current_option.info();
                            });

                        // TODO check if this correct
                        if (option_with_script != m_actions.cend() && !multiple_actions_allowed) {
                            // overwriting existing action
                            spdlog::get("logger")->info("Overwriting existing action");
                            option_with_script->m_option_info = current_option.info();
                        } else {
                            // adding additional action
                            spdlog::get("logger")->info("Adding existing action");
                            m_actions.emplace_back(current_option.info());
                            option_with_script = m_actions.end() - 1;
                        }

                        option_with_script->m_action_name = current_action.title();
                        option_with_script->m_script = script->value();
                        option_with_script->m_option_info = current_option.info();
                        option_with_script->m_last_value = current_option.value_as_variant();

                        auto init_range_values = [&current_action, &option_with_script](const auto &sane_value) {
                            using current_type = std::decay_t<decltype(sane_value)>;
                            if constexpr (std::is_same_v<current_type, int> ||
                                          std::is_same_v<current_type, sanepp::Fixed> ||
                                          std::is_same_v<current_type, bool>) {
                                auto ret = current_action.get<confusepp::Section>(Config::Constants::numerical_trigger);

                                if (!ret) {
                                    spdlog::get("logger")->warn("No trigger values were set");
                                    return;
                                }

                                if (auto int_value = ret->get<confusepp::Option<int>>(Config::Constants::from_value);
                                    int_value) {
                                    option_with_script->m_from_value = ActionValue<int>(int_value->value());
                                } else {
                                    spdlog::get("logger")->warn("No from value was set");
                                }

                                if (auto int_value = ret->get<confusepp::Option<int>>(Config::Constants::to_value);
                                    int_value) {
                                    option_with_script->m_to_value = ActionValue<int>(int_value->value());
                                } else {
                                    spdlog::get("logger")->warn("No from value was set");
                                }

                            } else if constexpr (std::is_same_v<current_type, std::string>) {
                                auto ret = current_action.get<confusepp::Section>(Config::Constants::string_trigger);

                                if (!ret) {
                                    spdlog::get("logger")->warn("No trigger values were set");
                                    return;
                                }

                                try {
                                    if (auto string_value =
                                            ret->get<confusepp::Option<std::string>>(Config::Constants::from_value);
                                        string_value) {
                                        option_with_script->m_from_value =
                                            ActionValue<std::string>(string_value->value());
                                    }

                                    if (auto string_value =
                                            ret->get<confusepp::Option<std::string>>(Config::Constants::to_value);
                                        string_value) {
                                        option_with_script->m_to_value =
                                            ActionValue<std::string>(string_value->value());
                                    }
                                } catch (std::regex_error) {
                                    // Error compiling regular expressions
                                }
                            }
                        };

                        if (current_option.value_as_variant()) {
                            std::visit(init_range_values, *current_option.value_as_variant());
                        } else {
                            spdlog::get("logger")->critical("Couldn't get value of current option");
                        }
                    }
                }
            }
        }
    }

    void SaneHandler::PollHandler::poll_device() {
        SignalHandler signal_handler;

        signal_handler.disable_signals_for_thread();

        auto device = device_info().open();

        if (!device) {
            spdlog::get("logger")->critical("Couldn't open device {0}", device_info().name());
            return;
        }

        Config config;
        auto global_section = config.get<confusepp::Section>(Config::Constants::global);

        if (!global_section) {
            spdlog::get("logger")->critical("Config is invalid");
            return;
        }

        find_matching_options(*device, *global_section);
        find_matching_functions(*device, *global_section);

        if (auto device_multi_section = config.get<confusepp::Multisection>(Config::Constants::device);
            device_multi_section) {
            for (auto device_section : device_multi_section->sections()) {
                auto device_filter = device_section.get<confusepp::Option<std::string>>(Config::Constants::filter);
                if (!device_filter) {
                    continue;
                }

                std::regex device_regex;

                try {
                    device_regex.assign(device_filter->value(), std::regex_constants::extended);
                } catch (std::regex_error) {
                    // Regex error
                    continue;
                }

                if (!std::regex_match(device_info().name(), device_regex)) {
                    continue;
                }

                auto local_actions = device_section.get<confusepp::Multisection>(Config::Constants::action);

                if (!local_actions) {
                    continue;
                }

                spdlog::get("logger")->info("Found local actions for device {0}", device_info().name());

                find_matching_options(*device, device_section);
                find_matching_options(*device, device_section);
            }
        }

        int timeout =
            config.get<confusepp::Option<int>>(Config::Constants::global / Config::Constants::timeout)->value();

        spdlog::get("logger")->info("Start polling for device {0}", device_info().name());
        while (!m_terminate) {
            // spdlog::get("logger")->info("Polling device {0}", device_info().name());
            for (auto current_action : m_actions) {
                auto current_value = device->find_option(current_action.m_option_info)->value_as_variant();

                if (!current_value) {
                    spdlog::get("logger")->warn("Couldn't get current value of option {0} of device {1}",
                                                current_action.m_option_info.name(), device_info().name());
                    continue;
                }

                if (!current_action.m_last_value) {
                    current_action.m_last_value = current_value;
                }

                auto has_value_changed = [&current_action](const auto &current_value) -> bool {
                    using type = std::decay_t<decltype(current_value)>;

                    if (!std::holds_alternative<type>(*current_action.m_last_value)) {
                        spdlog::get("logger")->critical("Type of action has changed should never happen");
                        return false;
                    }

                    if constexpr (std::is_same_v<type, int> || std::is_same_v<type, sanepp::Fixed> ||
                                  std::is_same_v<type, bool>) {
                        return std::get<ActionValue<int>>(current_action.m_to_value) == current_value &&
                               std::get<ActionValue<int>>(current_action.m_from_value) ==
                                   std::get<type>(current_action.m_last_value.value());
                    } else if constexpr (std::is_same_v<type, std::string>) {
                        return std::get<ActionValue<std::string>>(current_action.m_to_value) == current_value &&
                               std::get<ActionValue<std::string>>(current_action.m_from_value) ==
                                   std::get<type>(current_action.m_last_value.value());
                    } else {
                        spdlog::get("logger")->critical("Action has invalid type should never happen");
                    }
                    return false;
                };

                std::visit(
                    [&current_action](const auto &current_value) {
                        using type = std::decay_t<decltype(current_value)>;

                        if constexpr (std::is_same_v<type, int> || std::is_same_v<type, std::string> ||
                                      std::is_same_v<type, bool>) {
                            spdlog::get("logger")->info("{0} has value {1}", current_action.m_option_info.name(),
                                                        current_value);
                        }
                    },
                    *current_value);

                bool value_changed = std::visit(has_value_changed, *current_value);
                current_action.m_last_value = current_value;

                // TODO Handle script, also add event triggers that can be triggered from outside the thread
                if (value_changed) {
                    // Destroys current value of the optional thus freeing the resource (the device that the
                    // optional holds)
                    spdlog::get("logger")->info("Closing device {0}", device_info().name());
                    device.reset();

                    spdlog::get("logger")->info("Start script for device {0}", device_info().name());
                    // TODO start script

                    // Reopen device
                    spdlog::get("logger")->info("Reopen device {0}", device_info().name());
                    device = device_info().open();

                    if (!device) {
                        spdlog::get("logger")->critical("Couldn't reopen device");
                        return;
                    }

                    current_action.m_last_value.reset();
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        }
    }
}  // namespace scanbdpp
