#include "signal_handler.h"

#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include "run_configuration.h"
#include "config.h"
#include "sane.h"
#include "udev.h"

namespace scanbdpp {

    using sigaction_t = struct sigaction;

    // TODO reload global scanbd conf
    static void sig_hup_handler(int) {
        stop_sane_threads();
        start_sane_threads();

        load_config();
    }

    static void sig_usr1_handler(int) { stop_sane_threads(); }

    static void sig_usr2_handler(int) { start_sane_threads(); }

    static void sig_term_handler(int) {}

    void install_signal_handlers() {
        auto install_signal = [](int signal, const auto &signal_function,
                                 const std::initializer_list<int> &blocked_signals) -> int {
            sigaction_t action;
            std::memset(&action, 0, sizeof(struct sigaction));
            sigemptyset(&action.sa_mask);
            for (auto current_signal : blocked_signals) {
                sigaddset(&action.sa_mask, current_signal);
            }
            action.sa_handler = signal_function;

            return sigaction(signal, &action, nullptr);
        };

        if (install_signal(SIGHUP, sig_hup_handler, {SIGUSR1, SIGUSR2}) < 0) {
            exit(EXIT_FAILURE);
        }

        if (install_signal(SIGUSR1, sig_usr1_handler, {SIGUSR2, SIGHUP}) < 0) {
            exit(EXIT_FAILURE);
        }

        if (install_signal(SIGUSR2, sig_usr2_handler, {SIGUSR1, SIGHUP}) < 0) {
            exit(EXIT_FAILURE);
        }

        if (install_signal(SIGTERM, sig_term_handler, {}) < 0) {
            exit(EXIT_FAILURE);
        }

        if (install_signal(SIGINT, sig_term_handler, {}) < 0) {
            exit(EXIT_FAILURE);
        }
    }
}  // namespace scanbdpp
