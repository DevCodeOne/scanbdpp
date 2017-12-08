#include "common.h"

#include <cstdlib>

#include <string>
#include <vector>

#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "spdlog/spdlog.h"

#include "config.h"
#include "device_events.h"
#include "sane.h"
#include "signal.h"

namespace scanbdpp {

    void DeviceEvents::hook_device_ex(const std::string &parameter, const std::string &action_name,
                                      const std::string &device_name) {
        Config config;
        if (!config || !config.get<confusepp::Option<std::string>>(parameter)) {
            return;
        }

        std::vector<std::string> env_vars = environment();

        if (auto device = config.get<confusepp::Option<std::string>>(Config::Constants::global / Config::Constants::device); device) {
            env_vars.emplace_back(device->value() + "=" + device_name);
        }

        if (auto action = config.get<confusepp::Option<std::string>>(Config::Constants::global / Config::Constants::action); action) {
            env_vars.emplace_back(action->value() + "=" + action_name);
        }

        std::unique_ptr<const char *[]> environment_variables = std::make_unique<const char *[]>(env_vars.size() + 1);

        size_t index = 0;
        for (auto &current_env : env_vars) {
            environment_variables[index++] = current_env.c_str();
        }
        environment_variables[env_vars.size()] = nullptr;

        std::string script = config.get<confusepp::Option<std::string>>(parameter)->value();
        if (auto script_path = make_script_path_absolute(script); !script_path.empty()) {
            if (pid_t cpid = fork(); cpid > 0) {
                int status = 0;

                if (waitpid(cpid, &status, 0) < 0) {
                }

                if (WIFEXITED(status)) {
                }

                if (WIFSIGNALED(status)) {
                }
            } else if (cpid == 0) {
                if (execle(script_path.native().c_str(), script_path.native().c_str(), nullptr,
                           environment_variables.get()) < 0) {
                    // TODO log error
                }

                exit(EXIT_FAILURE);
            } else {
                // TODO log error couldn't fork
            }
        }
    }

    void DeviceEvents::hook_device_insert(const std::string &device_name) {
        // hook_device_insert
        hook_device_ex(Config::Constants::device_insert_script, "insert", device_name);
    }

    void DeviceEvents::hook_device_remove(const std::string &device_name) {
        // hook_device_remove
        hook_device_ex(Config::Constants::device_remove_script, "remove", device_name);
    }

    void DeviceEvents::device_added() {
        SaneHandler sane;
        sane.stop();
        hook_device_insert("dbus device");
        sane.start();
    }

    // TODO Look if the device is even used and only restart sane if it's necessary
    void DeviceEvents::device_removed() {
        SaneHandler sane;
        sane.stop();
        hook_device_remove("dbus device");
        sane.start();
    }
}  // namespace scanbdpp
