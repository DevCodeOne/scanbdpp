#include "common.h"

#include "signal_handler.h"

#include <cstdlib>

#include <signal.h>

#include "config.h"
#include "run_configuration.h"
#include "sane.h"
#include "udev.h"

namespace scanbdpp {

    void SignalHandler::sig_hup_handler(int) {
        SaneHandler sane;

        sane.stop();
        sane.start();

        Config conf;
        conf.reload_config();
    }

    void SignalHandler::sig_usr1_handler(int) {
        SaneHandler sane;
        sane.stop();
    }

    void SignalHandler::sig_usr2_handler(int) {
        SaneHandler sane;
        sane.start();
    }

    void SignalHandler::sig_term_handler(int) {}

    void SignalHandler::install() {
        if (_installed_signal_handlers) {
            return;
        }

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

        _installed_signal_handlers = true;
    }
}  // namespace scanbdpp