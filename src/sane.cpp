// clang-format off
#include "common.h"
#include <sys/wait.h>
// clang-format on

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
        std::lock_guard<std::mutex> device_guard(_device_mutex);
        if (_device_threads.size() != 0) {
            return;
        }

        sanepp::Sane sane_instance;
        auto devices = sane_instance.devices(true);
        for (auto device_info : devices) {
            spdlog::get("logger")->info("Starting polling thread for device {0}", device_info.name());
            _device_threads.emplace_back(std::make_unique<PollHandler>(sane_instance, device_info));
        }
    }

    void SaneHandler::stop() {
        std::lock_guard<std::mutex> device_guard(_device_mutex);

        if (_device_threads.size() == 0) {
            return;
        }

        spdlog::get("logger")->info("Stopping {0} polling threads", _device_threads.size());
        for (auto &current_handler : _device_threads) {
            // stopping poll thread for device
            current_handler->stop();

            if (current_handler->poll_thread().joinable()) {
                current_handler->poll_thread().join();
            }
        }

        _device_threads.clear();
    }

    void SaneHandler::trigger_action(const std::string &device_name, const std::string &action_name) {
        std::lock_guard<std::mutex> device_guard(_device_mutex);

        for (auto &current_handler : _device_threads) {
            if (current_handler->device_info().name() == device_name) {
                current_handler->trigger_action(action_name);
                break;
            }
        }
    }

    SaneHandler::PollHandler::PollHandler(sanepp::Sane instance, sanepp::DeviceInfo device_info)
        : m_instance(instance),
          m_device_info(device_info),
          m_terminate(false),
          m_poll_thread(std::bind(&SaneHandler::PollHandler::poll_device, this)) {}

    void SaneHandler::PollHandler::stop() { m_terminate = true; }

    const sanepp::DeviceInfo &SaneHandler::PollHandler::device_info() const { return m_device_info; }

    const std::atomic_bool &SaneHandler::PollHandler::should_stop() const { return m_terminate; }

    const std::thread &SaneHandler::PollHandler::poll_thread() const { return m_poll_thread; }

    std::thread &SaneHandler::PollHandler::poll_thread() { return m_poll_thread; }

    void SaneHandler::PollHandler::trigger_action(const std::string &action) {
        for (auto &current_action : m_actions) {
            if (current_action.action_name() == action) {
                current_action.set_trigger();
            }
        }
    }

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
                            // TODO check if this is correct
                            auto function_with_option = std::find_if(
                                m_functions.begin(), m_functions.end(), [&current_option](const Function &current) {
                                    return current.option_info() == current_option.info();
                                });
                            if (function_with_option != m_functions.end()) {
                                // Warning for overriding function
                                function_with_option->env(env->value());
                            } else {
                                m_functions.emplace_back(Function(current_option.info()).env(env->value()));
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
                        auto value = current_option.value_as_variant();
                        if (!std::regex_match(current_option.info().name(), action_regex)) {
                            continue;
                        }

                        if (!value || std::holds_alternative<sanepp::Group>(*value) ||
                            std::holds_alternative<sanepp::Button>(*value)) {
                            continue;
                        }

                        auto multiple_actions_allowed = config.get<confusepp::Option<bool>>(
                            Config::Constants::global / Config::Constants::multiple_actions);

                        auto option_with_script =
                            std::find_if(m_actions.begin(), m_actions.end(), [&current_option](const auto &action) {
                                return action.option_info() == current_option.info();
                            });

                        // TODO check if this correct
                        if (option_with_script != m_actions.cend() && !multiple_actions_allowed) {
                            spdlog::get("logger")->info(
                                "Overwriting existing action {0} with {1} for option {2} of device {3}",
                                option_with_script->action_name(), current_action.title(),
                                option_with_script->option_info().name(), device.info().name());
                            option_with_script->option_info(current_option.info());
                        } else {
                            spdlog::get("logger")->info("Adding new action {0} for option {1} of device {2}",
                                                        current_action.title(), current_option.info().name(),
                                                        device.info().name());
                            m_actions.emplace_back(current_option.info());
                            option_with_script = m_actions.end() - 1;
                        }

                        option_with_script->action_name(current_action.title());
                        option_with_script->script(script->value());
                        option_with_script->option_info(current_option.info());
                        option_with_script->last_value(current_option.value_as_variant());

                        auto init_range_values = [&current_action, &option_with_script](const auto &sane_value) {
                            using current_type = std::decay_t<decltype(sane_value)>;
                            if constexpr (std::is_same_v<current_type, int> ||
                                          std::is_same_v<current_type, sanepp::Fixed> ||
                                          std::is_same_v<current_type, bool>) {
                                auto trigger_section =
                                    current_action.get<confusepp::Section>(Config::Constants::numerical_trigger);

                                if (trigger_section) {
                                    if (auto int_value =
                                            trigger_section->get<confusepp::Option<int>>(Config::Constants::from_value);
                                        int_value) {
                                        option_with_script->from_value(ActionValue<int>(int_value->value()));
                                    } else {
                                        spdlog::get("logger")->warn("No from value was set");
                                    }

                                    if (auto int_value =
                                            trigger_section->get<confusepp::Option<int>>(Config::Constants::to_value);
                                        int_value) {
                                        option_with_script->to_value(ActionValue<int>(int_value->value()));
                                    } else {
                                        spdlog::get("logger")->warn("No to value was set");
                                    }
                                } else {
                                    spdlog::get("logger")->warn("No trigger values were set");
                                    option_with_script->from_value(
                                        ActionValue<int>(Config::Constants::from_value_def_int));
                                    option_with_script->to_value(ActionValue<int>(Config::Constants::to_value_def_int));
                                }
                            } else if constexpr (std::is_same_v<current_type, std::string>) {
                                auto trigger_section =
                                    current_action.get<confusepp::Section>(Config::Constants::string_trigger);

                                if (trigger_section) {
                                    try {
                                        if (auto string_value = trigger_section->get<confusepp::Option<std::string>>(
                                                Config::Constants::from_value);
                                            string_value) {
                                            option_with_script->from_value(
                                                ActionValue<std::string>(string_value->value()));
                                        }

                                        if (auto string_value = trigger_section->get<confusepp::Option<std::string>>(
                                                Config::Constants::to_value);
                                            string_value) {
                                            option_with_script->to_value(
                                                ActionValue<std::string>(string_value->value()));
                                        }
                                    } catch (std::regex_error) {
                                        // Error compiling regular expressions
                                        option_with_script->from_value(
                                            ActionValue<std::string>(Config::Constants::from_value_def_str));
                                        option_with_script->to_value(
                                            ActionValue<std::string>(Config::Constants::to_value_def_str));
                                    }
                                } else {
                                    spdlog::get("logger")->warn("No trigger values were set");
                                    option_with_script->from_value(
                                        ActionValue<std::string>(Config::Constants::from_value_def_str));
                                    option_with_script->to_value(
                                        ActionValue<std::string>(Config::Constants::to_value_def_str));
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
                find_matching_functions(*device, device_section);
            }
        }

        int timeout =
            config.get<confusepp::Option<int>>(Config::Constants::global / Config::Constants::timeout)->value();

        spdlog::get("logger")->info("Start polling for device {0}", device_info().name());
        while (!m_terminate) {
            for (auto current_action = m_actions.begin(); current_action != m_actions.end(); ++current_action) {
                // Only get a value once, because otherwise the backend might reset the value after
                // the value has been checked (Check original scanbd for reference)
                auto option_first_used =
                    std::find_if(m_actions.begin(), current_action, [&current_action](const auto &action) {
                        return current_action->option_info() == action.option_info();
                    });

                if (option_first_used != current_action) {
                    current_action->current_value(option_first_used->current_value());
                } else {
                    current_action->current_value(
                        device->find_option(current_action->option_info())->value_as_variant());
                }

                auto current_value = current_action->current_value();

                if (!current_value) {
                    spdlog::get("logger")->warn("Couldn't get current value of option {0} of device {1}",
                                                current_action->option_info().name(), device_info().name());
                    continue;
                }

                if (!current_action->last_value()) {
                    current_action->last_value(current_value);
                }

                auto has_value_changed = [&current_action](const auto &current_value) -> bool {
                    using type = std::decay_t<decltype(current_value)>;

                    if (!std::holds_alternative<type>(*current_action->last_value())) {
                        spdlog::get("logger")->critical("Type of action has changed should never happen");
                        return false;
                    }

                    if constexpr (std::is_same_v<type, int> || std::is_same_v<type, sanepp::Fixed> ||
                                  std::is_same_v<type, bool>) {
                        auto to_value = std::get<ActionValue<int>>(current_action->to_value());
                        auto from_value = std::get<ActionValue<int>>(current_action->from_value());
                        auto last_value = std::get<type>(current_action->last_value().value());

                        return to_value == current_value && from_value == last_value;
                    } else if constexpr (std::is_same_v<type, std::string>) {
                        auto to_value = std::get<ActionValue<std::string>>(current_action->to_value());
                        auto from_value = std::get<ActionValue<std::string>>(current_action->from_value());
                        auto last_value = std::get<type>(current_action->last_value().value());

                        return to_value == current_value && from_value == last_value;
                    } else {
                        spdlog::get("logger")->critical("Action has invalid type, this should never happen");
                    }
                    return false;
                };

                bool value_changed = std::visit(has_value_changed, *current_value);
                current_action->last_value(current_value);

                if (value_changed || current_action->is_triggered()) {
                    current_action->unset_trigger();
                    // Destroys current value of the optional thus freeing the resource (the device that the
                    // optional holds)
                    auto env_vars = environment();

                    if (auto device_env = config.get<confusepp::Option<std::string>>(
                            Config::Constants::global / Config::Constants::environment / Config::Constants::device);
                        device_env) {
                        env_vars.emplace_back(device_env->value() + "=" + device->info().name());
                    }

                    if (auto action_env = config.get<confusepp::Option<std::string>>(
                            Config::Constants::global / Config::Constants::environment / Config::Constants::action);
                        action_env) {
                        env_vars.emplace_back(action_env->value() + "=" + current_action->action_name());
                    }
                    for (auto current_function : m_functions) {
                        auto option_first_used =
                            std::find_if(m_actions.cbegin(), m_actions.cend(), [&current_function](const auto &action) {
                                return current_function.option_info() == action.option_info();
                            });

                        std::optional<sanepp::Option::value_type> current_value;

                        if (option_first_used != m_actions.cend()) {
                            current_value = option_first_used->current_value();
                        } else {
                            current_value = device->find_option(current_function.option_info())->value_as_variant();
                        }

                        if (!current_value) {
                            continue;
                        }

                        std::visit(
                            [&env_vars, &current_function](const auto &value) {
                                using type = std::decay_t<decltype(value)>;

                                std::string as_string;

                                if constexpr (std::is_same_v<type, int> || std::is_same_v<type, bool>) {
                                    as_string = std::to_string(value);
                                } else if constexpr (std::is_same_v<type, sanepp::Fixed>) {
                                    as_string = std::to_string(value.value());
                                } else if constexpr (std::is_same_v<type, std::string>) {
                                    as_string = value;
                                } else {
                                    return;
                                }

                                env_vars.emplace_back(current_function.env() + "=" + as_string);
                            },
                            *current_value);
                    }

                    spdlog::get("logger")->info("Closing device {0}", device_info().name());
                    device.reset();

                    spdlog::get("logger")->info("Start script for device {0}", device_info().name());

                    std::unique_ptr<const char *[]> environment_variables =
                        std::make_unique<const char *[]>(env_vars.size() + 1);

                    size_t index = 0;
                    for (auto &current_env : env_vars) {
                        environment_variables[index++] = current_env.c_str();
                    }
                    environment_variables[env_vars.size()] = nullptr;

                    auto script_absolute_path = make_script_path_absolute(current_action->script());

                    using namespace std::string_literals;
                    if (current_action->action_name() != ""s) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));

                        pid_t cpid;

                        if ((cpid = fork()) < 0) {
                            spdlog::get("logger")->critical("Can't fork {0}", strerror(errno));
                        } else if (cpid > 0) {
                            spdlog::get("logger")->info("Waiting for child {0}", script_absolute_path.c_str());
                            int status = 0;

                            if (waitpid(cpid, &status, 0) < 0) {
                                spdlog::get("logger")->critical("waitpid: {0}", script_absolute_path.c_str());
                            }

                            if (WIFEXITED(status)) {
                                spdlog::get("logger")->info("Child {0} exited with status: {1}",
                                                            script_absolute_path.c_str(), WEXITSTATUS(status));
                            }

                            if (WIFSIGNALED(status)) {
                                spdlog::get("logger")->info("Child {0} signaled with signal: {1}",
                                                            script_absolute_path.c_str(), WTERMSIG(status));
                            }
                        } else {
                            // TODO add the rest

                            if (execle(script_absolute_path.c_str(), script_absolute_path.c_str(), NULL,
                                       environment_variables.get()) < 0) {
                                spdlog::get("logger")->critical("execle: {0}", strerror(errno));
                            }

                            exit(EXIT_FAILURE);
                        }
                    }

                    // Reopen device
                    spdlog::get("logger")->info("Reopen device {0}", device_info().name());
                    device = device_info().open();

                    if (!device) {
                        spdlog::get("logger")->critical("Couldn't reopen device");
                        return;
                    }

                    current_action->last_value(std::optional<sanepp::Option::value_type>{});
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        }
        spdlog::get("logger")->info("Stopped polling device {0}", device->info().name());
    }
}  // namespace scanbdpp
