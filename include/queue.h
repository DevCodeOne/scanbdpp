#pragma once

#include <atomic>

#include "defines.h"

class QueueHandler {
   public:
    void start() const;
    void stop() const;

   private:
    static void queue_thread();

    static inline bool _thread_started = false;
    static inline std::atomic_bool _thread_stop = false;
    static inline std::thread _thread_inst;
    static inline const char queue_name[] = QUEUE_NAME;
    static inline constexpr unsigned int _max_message_size = PIPE_BUF * 2;
    static inline constexpr unsigned int _max_messages = 1024;
};
