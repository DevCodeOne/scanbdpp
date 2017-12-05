#pragma once

#include "common.h"

#include <atomic>
#include <cstring>
#include <initializer_list>

#include <signal.h>

namespace scanbdpp {
    class SignalHandler {
       public:
        void install();
        void disable_signals_for_thread();
        const std::atomic_bool &should_exit() const;

       private:
        static void sig_hup_handler(int);
        static void sig_usr1_handler(int);
        static void sig_usr2_handler(int);
        static void sig_term_handler(int);
        template<typename T>
        int install_signal(int signal, const T &signal_function, const std::initializer_list<int> &blocked_signals);

        static inline bool _installed_signal_handlers = false;
        static inline std::atomic_bool _should_exit = false;
    };

    template<typename T>
    int SignalHandler::install_signal(int signal, const T &signal_function,
                                      const std::initializer_list<int> &blocked_signals) {
        struct sigaction action;
        std::memset(&action, 0, sizeof(struct sigaction));
        sigemptyset(&action.sa_mask);
        for (auto current_signal : blocked_signals) {
            sigaddset(&action.sa_mask, current_signal);
        }
        action.sa_handler = signal_function;

        return sigaction(signal, &action, nullptr);
    }

}  // namespace scanbdpp
