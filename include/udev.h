#pragma once

#include <atomic>
#include <mutex>
#include <thread>

namespace scanbdpp {
    class UDevHandler {
       public:
        void start();
        void stop();

       private:
        static void udev_thread();

        static inline constexpr char add_action[] = "add";
        static inline constexpr char remove_action[] = "remove";
        static inline constexpr char device_type[] = "usb_device";
        static inline constexpr std::chrono::duration no_device_sleep = std::chrono::seconds(1);
        static inline bool _thread_started = false;
        static inline std::atomic_bool _thread_stop = false;
        static inline std::thread _thread_inst;
    };
}  // namespace scanbdpp
