#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "sanepp.h"

namespace scanbdpp {
    class SaneHandler {
       public:
        void start();
        void stop();

       private:
        static void poll_device(sanepp::Sane instance, sanepp::DeviceInfo device_info);

        static inline std::mutex _device_mutex;
        static inline std::vector<std::thread> _device_threads;
        static inline std::atomic_bool _terminate_threads = false;
    };
}  // namespace scanbdpp
