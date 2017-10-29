#include "common.h"

#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

#include "cxxopts.hpp"

#include "config.h"
#include "daemonize.h"
#include "run_configuration.h"
#include "sane.h"
#include "udev.h"

// TODO what is trigger_device and trigger_action
int main(int argc, char *argv[]) {
    using namespace scanbdpp;
    using confusepp::List;
    using confusepp::Option;

    cxxopts::Options options("scanbd", "scanbd is a scanner button daemon");

    options.add_options()("m,manager", "start in manager mode")
        ("s,signal", "start in signal mode")
        ("d,debug", "enable debugging", cxxopts::value<int>())
        ("f,foreground", "start in foreground")
        ("c,config", "provide custom config file", cxxopts::value<std::string>())
        ("t,trigger", "add trigger for device", cxxopts::value<int>())
        ("a,action", "trigger action number", cxxopts::value<int>())
        ("h,help", "print this help menu");

    try {
        options.parse(argc, argv);

        if (options.count("help")) {
            std::cout << options.help() << std::endl;
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

        // TODO implement
        if (options.count("trigger")) {
        }

        // TODO implement
        if (options.count("action")) {
        }
    } catch (cxxopts::option_not_exists_exception e) {
        std::cout << e.what() << '\n' << options.help() << std::endl;
        return EXIT_FAILURE;
    } catch (cxxopts::option_requires_argument_exception e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (cxxopts::argument_incorrect_type e) {
        std::cout << "The provided argument is of the wrong type" << std::endl;
        return EXIT_FAILURE;
    }

    // load_config();

    start_sane_threads();

    std::this_thread::sleep_for(std::chrono::seconds(20));

    stop_sane_threads();

    //
    //     if (auto value = config->get<Option<int>>("/global/debug"); value) {
    //         run_config.debug(run_config.debug() | value->value());
    //     }
    //
    //     if (run_config.debug()) {
    //         if (auto value = config->get<Option<int>>("/global/debug_level"); value) {
    //             run_config.debug_level(value->value());
    //         }
    //     }
    //
    //     using namespace std::string_literals;
    //     if ("scanbm"s == argv[0]) {
    //         run_config.manager_mode(true);
    //     }
    //
    //     if (run_config.manager_mode()) {
    //         pid_t scanbd_pid = -1;
    //         auto scanbd_pid_path = config->get<Option<std::string>>("global/saned");
    //
    //         if (!scanbd_pid_path) {
    //             // TODO assert or something similiar
    //         }
    //
    //         if (run_config.signal()) {
    //             std::ifstream scanbd_pid_file(scanbd_pid_path->value());
    //
    //             if (!scanbd_pid_file) {
    //                 // Can't read from pid-file
    //             }
    //
    //             scanbd_pid_file >> scanbd_pid;
    //
    //             if (kill(scanbd_pid, SIGUSR1) < 0) {
    //                 // Can't send Signal
    //                 exit(EXIT_FAILURE);
    //             }
    //
    //             using namespace std::chrono_literals;
    //             std::this_thread::sleep_for(1s);
    //         } else {
    //             // TODO Implement dbus backend
    //         }
    //
    //         if (pid_t spid = fork(); spid < 0) {
    //             // Couldn't fork
    //             exit(EXIT_FAILURE);
    //         } else if (spid > 0) {
    //             int status = 0;
    //
    //             if (waitpid(spid, &status, 0) < 0) {
    //                 // Waiting for saned failed
    //                 exit(EXIT_FAILURE);
    //             }
    //
    //             if (WIFEXITED(status)) {
    //                 // Sane exited with status
    //             }
    //             if (WIFSIGNALED(status)) {
    //                 // Sane exited due to signal
    //             }
    //
    //             if (run_config.signal()) {
    //                 using namespace std::chrono_literals;
    //                 std::this_thread::sleep_for(1s);
    //
    //                 if (scanbd_pid > 0) {
    //                     if (kill(scanbd_pid, SIGUSR2) < 0) {
    //                         // Can't send signal
    //                     }
    //                 }
    //             } else {
    //                 // TODO Implement dbus
    //             }
    //         } else {
    //             std::string saned;
    //             if (auto value = config->get<Option<std::string>>("/global/saned"); value) {
    //                 saned = value->value();
    //             } else {
    //                 // Saned not set error
    //                 exit(EXIT_FAILURE);
    //             }
    //             if (getenv("LISTEND_PID") != nullptr) {
    //                 std::string listen_fds = std::to_string((long)getpid());
    //                 setenv("LISTEN_PID", listen_fds.c_str(), 1);
    //                 // Systemd detected: Updating LISTEN_PID env. variable
    //             }
    //
    //             if (auto env_list = config->get<Option<List<std::string>>>("/global/saned_env"); env_list) {
    //                 for (auto env : env_list->value()) {
    //                     std::istringstream env_stream(env);
    //
    //                     std::string variable;
    //                     std::string value;
    //                     if (std::getline(env_stream, variable, '=') && std::getline(env_stream, value, '=')) {
    //                         if (setenv(variable.c_str(), value.c_str(), 1) < 0) {
    //                             // Couldn't set environment variable
    //                         } else {
    //                             // Did set evironment variable
    //                         }
    //                     } else {
    //                         // Malformend env
    //                     }
    //                 }
    //             }
    //
    //             if (setsid() < 0) {
    //                 // setsid : Error
    //             }
    //             if (execl(saned.c_str(), "saned", nullptr) < 0) {
    //                 // exec of sane failed
    //                 exit(EXIT_FAILURE);
    //             }
    //
    //             // Not reached
    //             exit(EXIT_FAILURE);
    //         }
    //     } else {
    //         if (!run_config.foreground()) {
    //             // daemonize
    //             daemonize();
    //         }
    //
    //         std::string euser = "";
    //         std::string egroup = "";
    //         if (auto value = config->get<Option<std::string>>("/global/user"); value) {
    //             euser = value->value();
    //         }
    //         if (auto value = config->get<Option<std::string>>("/global/group"); value) {
    //             egroup = value->value();
    //         }
    //
    //         using namespace std::string_literals;
    //         if (euser == ""s || egroup == ""s) {
    //             // Error no user or group defined
    //         }
    //
    //         // Dropping privileges to uid
    //         struct passwd *pwd = getpwnam(euser.c_str());
    //
    //         if (pwd == nullptr) {
    //             if (errno != 0) {
    //                 // No user + errno
    //             } else {
    //                 // No user
    //             }
    //             exit(EXIT_FAILURE);
    //         }
    //
    //         // Dropping privileges to gid
    //         struct group *grp = getgrnam(egroup.c_str());
    //
    //         if (grp == nullptr) {
    //             if (errno != 0) {
    //                 // No group + errno
    //             } else {
    //                 // No group
    //             }
    //             exit(EXIT_FAILURE);
    //         }
    //
    //         std::string scanbd_pid_path = "";
    //
    //         if (auto value = config->get<Option<std::string>>("/global/pidfile")) {
    //             scanbd_pid_path = value->value();
    //         }
    //
    //         if (!run_config.foreground()) {
    //             int pid_fd =
    //                 open(scanbd_pid_path.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    //
    //             if (pid_fd < 0) {
    //                 // Couldn't create pidfile
    //                 exit(EXIT_FAILURE);
    //             }
    //
    //             if (ftruncate(pid_fd, 0) < 0) {
    //                 // Couldn't clear pidfile
    //                 exit(EXIT_FAILURE);
    //             }
    //             std::string pid_string = std::to_string(getpid()).c_str();
    //             if (write(pid_fd, pid_string.c_str(), pid_string.size()) < 0) {
    //                 // Couldn't write to pidfile
    //                 exit(EXIT_FAILURE);
    //             }
    //
    //             if (close(pid_fd) < 0) {
    //                 // Couldn't close pidfile
    //                 exit(EXIT_FAILURE);
    //             }
    //
    //             if (chown(scanbd_pid_path.c_str(), pwd->pw_uid, grp->gr_gid) < 0) {
    //                 // Can't chown pidfile
    //                 exit(EXIT_FAILURE);
    //             }
    //         }
    //
    //         unsigned int index = 0;
    //
    //         if (grp != nullptr) {
    //             while (grp->gr_mem) {
    //                 if (index == 0) {
    //                     // Group has member grp->gr_name
    //                 }
    //                 // grp->gr_mem[index]
    //                 ++index;
    //             }
    //             // Drop privileges to gid (grp->gr_gid)
    //             if (setegid(grp->gr_gid) < 0) {
    //                 // Coulnd't set the effective gid (grp->gr_gid)
    //             } else {
    //                 // Running as effective gid (grp->gr_gid)
    //             }
    //         }
    //
    //         if (pwd != nullptr) {
    //             // Drop privileges to uid
    //             if (seteuid(pwd->pw_uid) < 0) {
    //                 setgroups(0, nullptr);
    //                 // Couldn't set the effective uid to pwd->pw_uid
    //             } else {
    //                 // Running as effective uid pwd->pw_uid
    //             }
    //         }
    //
    //         if (getenv("SANE_CONFIG_DIR") != nullptr) {
    //             // SANE_CONFIG_DIR = (SANE_CONFIG_DIR)
    //         } else {
    //             // SANE_CONFIG_DIR not set
    //         }
    //
    //         start_sane_threads();
    //         start_udev_thread();
    //
    //         while (true) {
    //             if (pause() < 0) {
    //                 // Pause (strerror)
    //             }
    //         }
    //     }
    return EXIT_SUCCESS;
}
