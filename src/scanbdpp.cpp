// clang-format off
#include "common.h"
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
// clang-format on

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"

#include "config.h"
#include "daemonize.h"
#include "pipe.h"
#include "run_configuration.h"
#include "sane.h"
#include "signal_handler.h"
#include "udev.h"

using confusepp::List;
using confusepp::Option;

void die(int exit_code) { exit(exit_code); }

int main(int argc, char *argv[]) {
    using namespace scanbdpp;

    SignalHandler signals;
    signals.install();

    auto logger = spdlog::stdout_color_mt("logger");
    RunConfiguration run_config;

    cxxopts::Options options("scanbd", "scanbd is a scanner button daemon");

    // clang-format off
    options.add_options()
        ("m,manager", "start in manager mode")
        ("s,signal", "start in signal mode")
        ("d,debug", "enable debugging", cxxopts::value<int>())
        ("f,foreground", "start in foreground")
        ("c,config", "provide custom config file", cxxopts::value<std::string>())
        ("t,trigger", "which device to trigger (use in combination with action)", cxxopts::value<std::string>())
        ("a,action", "which action to use", cxxopts::value<std::string>())
        ("h,help", "print this help menu");
    // clang-format on

    try {
        options.parse(argc, argv);

        if (options.count("help")) {
            std::cout << options.help() << std::endl;
            die(EXIT_SUCCESS);
        }

        run_config.manager_mode(options.count("manager"));
        run_config.signal(options.count("signal"));
        run_config.foreground(options.count("foreground"));

        if (options.count("debug")) {
            run_config.debug(true);
            run_config.debug_level(options["debug"].as<int>());
        }

        if (options.count("config")) {
            run_config.config_path(options["config"].as<std::string>());
        }

        // TODO check if trigger or device is number for legacy support
        if (options.count("trigger") && options.count("action")) {
            PipeHandler handler;

            std::string device = options["trigger"].as<std::string>();
            std::string action = options["action"].as<std::string>();
            std::string message = device + "," + action;

            handler.write_message(message);

            die(EXIT_SUCCESS);
        } else if ((options.count("trigger") == 1) ^ (options.count("action") == 1)) {
            if (options.count("trigger") == 0) {
                std::cout << "No device was specified to trigger" << std::endl;
            }

            if (options.count("action") == 0) {
                std::cout << "No action was specified to execute" << std::endl;
            }

            die(EXIT_FAILURE);
        }

    } catch (cxxopts::option_not_exists_exception e) {
        std::cout << "Option does not exist" << '\n' << e.what() << std::endl;
        return (EXIT_FAILURE);
    } catch (cxxopts::option_requires_argument_exception e) {
        std::cout << "Option requires an argument" << '\n' << e.what() << std::endl;
        return (EXIT_FAILURE);
    } catch (cxxopts::argument_incorrect_type e) {
        std::cout << "The argument provided for the option is of wrong type" << '\n' << e.what() << std::endl;
        return (EXIT_FAILURE);
    }

    Config config;
    PipeHandler pipe;
    SaneHandler sane;
    UDevHandler udev;

    if (auto value = config.get<Option<int>>(Config::Constants::global / Config::Constants::debug); value) {
        run_config.debug(run_config.debug() | value->value());
    }

    if (run_config.debug()) {
        if (auto value = config.get<Option<int>>(Config::Constants::global / Config::Constants::debug_level); value) {
            run_config.debug_level(value->value());
        }
    }

    using namespace std::string_literals;
    if ("scanbm"s == argv[0]) {
        run_config.manager_mode(true);
    }

    if (run_config.manager_mode()) {
        pid_t scanbd_pid = -1;
        auto scanbd_pid_path = config.get<Option<std::string>>(Config::Constants::global / Config::Constants::saned);

        if (!scanbd_pid_path) {
            // TODO assert or something similiar
        }

        if (run_config.signal()) {
            std::ifstream scanbd_pid_file(scanbd_pid_path->value());

            if (!scanbd_pid_file) {
                // Can't read from pid-file
            }

            scanbd_pid_file >> scanbd_pid;

            if (kill(scanbd_pid, SIGUSR1) < 0) {
                // Can't send Signal
                spdlog::get("logger")->critical("Can't send signal to stop polling threads");
                die(EXIT_FAILURE);
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            spdlog::get("logger")->critical("Dbus is not supported");
            die(EXIT_FAILURE);
        }

        if (pid_t spid = fork(); spid < 0) {
            spdlog::get("logger")->critical("Couldn't fork");
            die(EXIT_FAILURE);
        } else if (spid > 0) {
            int status = 0;

            if (waitpid(spid, &status, 0) < 0) {
                spdlog::get("logger")->critical("Waiting for saned failed");
                die(EXIT_FAILURE);
            }

            if (WIFEXITED(status)) {
                spdlog::get("logger")->info("Sane exited normally");
            }
            if (WIFSIGNALED(status)) {
                spdlog::get("logger")->info("Sane exited due to signal");
            }

            if (run_config.signal()) {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1s);

                if (scanbd_pid > 0) {
                    if (kill(scanbd_pid, SIGUSR2) < 0) {
                        // Can't send signal
                        spdlog::get("logger")->critical("Can't send signal to start polling threads");
                    }
                }
            } else {
                spdlog::get("logger")->critical("Dbus is not supported");
                die(EXIT_FAILURE);
            }
        } else {
            std::string saned;
            if (auto value = config.get<Option<std::string>>(Config::Constants::global / Config::Constants::saned);
                value) {
                saned = value->value();
            } else {
                spdlog::get("logger")->critical("Path to saned is not set");
                die(EXIT_FAILURE);
            }
            if (getenv("LISTEND_PID") != nullptr) {
                std::string listen_fds = std::to_string((long)getpid());
                setenv("LISTEN_PID", listen_fds.c_str(), 1);
                spdlog::get("logger")->info("Systemd detected: Updating LISTEN_PID env. variable");
            }

            if (auto env_list =
                    config.get<Option<List<std::string>>>(Config::Constants::global / Config::Constants::saned_envs);
                env_list) {
                for (auto env : env_list->value()) {
                    std::istringstream env_stream(env);

                    std::string variable;
                    std::string value;
                    if (std::getline(env_stream, variable, '=') && std::getline(env_stream, value, '=')) {
                        if (setenv(variable.c_str(), value.c_str(), 1) < 0) {
                            spdlog::get("logger")->critical("Couldn't set environment variable");
                        } else {
                            spdlog::get("logger")->info("Environment variable were updated");
                        }
                    } else {
                        spdlog::get("logger")->warn("Malformed environment variables in config file");
                    }
                }
            }

            if (setsid() < 0) {
                spdlog::get("logger")->critical("Error setting process id group {0}", strerror(errno));
            }
            if (execl(saned.c_str(), "saned", nullptr) < 0) {
                spdlog::get("logger")->critical("execl with saned failed");
                die(EXIT_FAILURE);
            }

            die(EXIT_FAILURE);
        }
    } else {
        if (!run_config.foreground()) {
            spdlog::get("logger")->info("Daemonizing scanbd");
            if (!daemonize()) {
                return EXIT_FAILURE;
            }
        }

        std::string euser;
        std::string egroup;
        if (auto value = config.get<Option<std::string>>(Config::Constants::global / Config::Constants::user); value) {
            euser = value->value();
        }
        if (auto value = config.get<Option<std::string>>(Config::Constants::global / Config::Constants::group); value) {
            egroup = value->value();
        }

        using namespace std::string_literals;
        if (euser == ""s || egroup == ""s) {
            spdlog::get("logger")->critical("No user or group defined");
        }

        spdlog::get("logger")->info("Dropping privileges to uid");
        struct passwd *pwd = getpwnam(euser.c_str());

        if (pwd == nullptr) {
            if (errno != 0) {
                spdlog::get("logger")->critical("No user {0} {1}", euser, strerror(errno));
            } else {
                spdlog::get("logger")->critical("No user {0}", euser);
            }
            die(EXIT_FAILURE);
        }

        spdlog::get("logger")->info("Dropping privileges to gid");
        struct group *grp = getgrnam(egroup.c_str());

        if (grp == nullptr) {
            if (errno != 0) {
                spdlog::get("logger")->critical("No group {0} {1}", egroup, strerror(errno));
            } else {
                spdlog::get("logger")->critical("No group {0}", egroup);
            }
            die(EXIT_FAILURE);
        }

        std::string scanbd_pid_path;

        if (auto value = config.get<Option<std::string>>(Config::Constants::global / Config::Constants::pidfile)) {
            scanbd_pid_path = value->value();
        }

        if (!run_config.foreground()) {
            int pid_fd =
                open(scanbd_pid_path.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

            if (pid_fd < 0) {
                spdlog::get("logger")->critical("Couldn't create pidfile {0}", strerror(errno));
                die(EXIT_FAILURE);
            }

            if (ftruncate(pid_fd, 0) < 0) {
                spdlog::get("logger")->critical("Couldn't clear pidfile {0}", strerror(errno));
                die(EXIT_FAILURE);
            }
            std::string pid_string = std::to_string(getpid());
            if (write(pid_fd, pid_string.c_str(), pid_string.size()) < 0) {
                spdlog::get("logger")->critical("Couldn't write to pidfile {0}", strerror(errno));
                die(EXIT_FAILURE);
            }

            if (close(pid_fd) < 0) {
                spdlog::get("logger")->critical("Couldn't close pidfile {0}", strerror(errno));
                die(EXIT_FAILURE);
            }

            if (chown(scanbd_pid_path.c_str(), pwd->pw_uid, grp->gr_gid) < 0) {
                spdlog::get("logger")->critical("Couldn't chown pidfile {0}", strerror(errno));
                die(EXIT_FAILURE);
            }
        }

        if (grp != nullptr) {
            unsigned int index = 0;
            while (grp->gr_mem[index]) {
                if (index == 0) {
                    spdlog::get("logger")->info("Group {0} has member : ", grp->gr_name);
                }
                spdlog::get("logger")->info("{0}", grp->gr_mem[index]);
                ++index;
            }
            spdlog::get("logger")->info("Dropping privileges to gid: {0}", grp->gr_gid);
            if (setegid(grp->gr_gid) < 0) {
                spdlog::get("logger")->warn("Couldn't set the effective gid to {0}", grp->gr_gid);
            } else {
                spdlog::get("logger")->info("Running as effective gid {0}", grp->gr_gid);
            }
        }

        if (pwd != nullptr) {
            spdlog::get("logger")->info("Dropping privileges to uid: {0}", pwd->pw_uid);
            if (seteuid(pwd->pw_uid) < 0) {
                setgroups(0, nullptr);
                spdlog::get("logger")->warn("Couldn't set effective uid to {0}", pwd->pw_uid);
            } else {
                spdlog::get("logger")->info("Running as effective uid {0}", grp->gr_gid);
            }
        }

        if (getenv("SANE_CONFIG_DIR") != nullptr) {
            // SANE_CONFIG_DIR = (SANE_CONFIG_DIR)
        } else {
            spdlog::get("logger")->warn("SANE_CONFIG_DIR not set");
        }

        if (!signals.should_exit()) {
            sane.start();
            udev.start();
            pipe.start();
        }

        while (true) {
            if (signals.should_exit()) {
                sane.stop();
                udev.stop();
                pipe.stop();
                spdlog::get("logger")->info("Exiting scanbd");
                spdlog::drop_all();
                return EXIT_SUCCESS;
            }

            if (pause() < 0) {
                if (errno != EINTR) {
                    spdlog::get("logger")->warn("pause() error {0}", strerror(errno));
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
