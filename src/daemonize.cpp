#include "common.h"

#include <cstdlib>
#include <initializer_list>

#include "daemonize.h"

namespace scanbdpp {
    void daemonize() {
        if (pid_t pid = fork(); pid < 0) {
            // Couldn't fork
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            exit(EXIT_SUCCESS);
        }

        if (setsid() < 0) {
            // setsid error
            exit(EXIT_SUCCESS);
        }

        if (pid_t pid = fork(); pid < 0) {
            // Coulnd't fork
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            exit(EXIT_SUCCESS);
        }

        int ofd = -1;

        if (ofd = open("/dev/null", O_RDWR); ofd < 0) {
            // Coulnd't open /dev/null
            exit(EXIT_FAILURE);
        }

        auto point_filedes_to = [ofd](int fd) {
            if (dup2(ofd, fd) < 0) {
                // Couldn't set fd to ofd
                exit(EXIT_SUCCESS);
            }
        };

        for (auto des : std::initializer_list<int>{STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO}) {
            point_filedes_to(des);
        }

        if (chdir("/") < 0) {
            // Couldn't change working directory
            exit(EXIT_FAILURE);
        }
    }
}  // namespace scanbdpp
