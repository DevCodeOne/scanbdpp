#include "common.h"
#include "device_events.h"

#include <cstdlib>

#include <string>
#include <vector>

#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "config.h"
#include "sane.h"
#include "signal.h"

namespace scanbdpp {


    void DeviceEvents::hook_device_ex(const std::string &parameter, const std::string &action_name,
                                 const std::string &device_name) {
        Config config;
        if (!config || !config.get<confusepp::Option<std::string>>(parameter)) {
            return;
        }

        std::vector<char *> environment_variables;

        std::string current_env = "PATH";
        std::experimental::filesystem::path root = "global";

        if (getenv(current_env.c_str())) {
            environment_variables.emplace_back(strdup(getenv(current_env.c_str())));
        } else {
            environment_variables.emplace_back(strdup("/usr/sbin:/usr/bin:/sbin:/bin"));
        }

        current_env = "PWD";

        if (getenv(current_env.c_str())) {
            environment_variables.emplace_back(strdup(getenv(current_env.c_str())));
        } else {
            char buffer[path_max];
            char *working_directory = getcwd(buffer, path_max - 1);

            if (working_directory) {
                environment_variables.emplace_back(strdup(working_directory));
            } else {
                // TODO log couldn't get working directory
            }
        }

        current_env = "USER";

        if (getenv(current_env.c_str())) {
            environment_variables.emplace_back(strdup(getenv(current_env.c_str())));
        } else {
            passwd *pwd = getpwuid(geteuid());
            environment_variables.emplace_back(strdup(pwd->pw_name));
        }

        current_env = "HOME";

        if (getenv(current_env.c_str())) {
            environment_variables.emplace_back(strdup(getenv(current_env.c_str())));
        } else {
            passwd *pwd = getpwuid(geteuid());
            environment_variables.emplace_back(strdup(pwd->pw_dir));
        }

        if (auto device = config.get<confusepp::Option<std::string>>(root / "device"); device) {
            std::ostringstream device_environment;
            device_environment << device->value() << "=" << device_name;
            environment_variables.emplace_back(strdup(device_environment.str().c_str()));
        }

        if (auto action = config.get<confusepp::Option<std::string>>(root / "action"); action) {
            std::ostringstream action_environment;
            action_environment << action->value() << "=" << action_name;
            environment_variables.emplace_back(strdup(action_environment.str().c_str()));
        }

        environment_variables.emplace_back(nullptr);

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
                           environment_variables.data()) < 0) {
                    // TODO log error
                }

                exit(EXIT_FAILURE);
            } else {
                // TODO log error couldn't fork
            }
        }

        // free environment_variables ? in original program script_path gets freed
        for (auto environment_variable : environment_variables) {
            std::free(environment_variable);
        }
    }

    void DeviceEvents::hook_device_insert(const std::string &device_name) { hook_device_ex("", "insert", device_name); }

    void DeviceEvents::hook_device_remove(const std::string &device_name) { hook_device_ex("", "remove", device_name); }

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
