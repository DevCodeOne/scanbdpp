#pragma once

#include "common.h"

#include <atomic>
#include <experimental/filesystem>
#include <mutex>

#include "defines.h"

class PipeHandler {
   public:
    PipeHandler();
    ~PipeHandler();
    void start() const;
    void stop() const;

    void write_message(const std::string &message) const;

   private:
    static void pipe_thread();

    static inline bool _thread_started = false;
    static inline std::atomic_bool _thread_stop = false;
    static inline std::thread _thread_inst;
    static inline constexpr size_t _max_message_size = PIPE_BUF;
    static inline const std::experimental::filesystem::path pipe_path = PIPE_PATH;
    static inline std::mutex _instance_mutex;
    static inline unsigned int _instance_count = 0;
};
