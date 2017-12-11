// clang-format off
#include "common.h"
#include <errno.h>
#include <string.h>
// clang-format on

#include <cstdlib>
#include <initializer_list>

#include "spdlog/spdlog.h"

#include "daemonize.h"

namespace scanbdpp {
    bool daemonize() {
        if (pid_t pid = fork(); pid < 0) {
            spdlog::get("logger")->critical("Couldn't fork {0}", strerror(errno));
            return false;
        } else if (pid > 0) {
            exit(EXIT_SUCCESS);
        }

        if (setsid() < 0) {
            spdlog::get("logger")->critical("setsid error {0}", strerror(errno));
            return false;
        }

        if (pid_t pid = fork(); pid < 0) {
            spdlog::get("logger")->critical("Couldn't fork {0}", strerror(errno));
            return false;
        } else if (pid > 0) {
            exit(EXIT_SUCCESS);
        }

        int ofd = -1;

        if (ofd = open("/dev/null", O_RDWR); ofd < 0) {
            spdlog::get("logger")->warn("Couldn't open /dev/null {0}", strerror(errno));
            return false;
        }

        auto point_filedes_to = [ofd](int fd) {
            if (dup2(ofd, fd) < 0) {
                spdlog::get("logger")->warn("Couldn't set filedescriptor {0}", strerror(errno));
            }
        };

        for (auto des : std::initializer_list<int>{STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO}) {
            point_filedes_to(des);
        }

        if (chdir("/") < 0) {
            spdlog::get("logger")->warn("Couldn't change working directory", strerror(errno));
        }

        return true;
    }
}  // namespace scanbdpp
