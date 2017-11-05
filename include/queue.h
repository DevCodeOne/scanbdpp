#pragma once

class QueueHandler {
   public:
    void start();
    void stop();

   private:
    static void queue_thread();

    static inline bool _thread_started = false;
    static inline std::atomic_bool _thread_stop = false;
    static inline std::thread _thread_inst;
};
