#pragma once

#include <atomic>
#include <mutex>
#include <thread>

namespace scanbdpp {
    class UDevHandler {
       public:
        UDevHandler();
        ~UDevHandler();

        void start() const;
        void stop() const;

        class Constants {
           public:
            Constants() = delete;

            static inline constexpr char add_action[] = "add";
            static inline constexpr char remove_action[] = "remove";
            static inline constexpr char device_type[] = "usb_device";
            static inline constexpr std::chrono::duration no_device_sleep = std::chrono::seconds(1);
        };

       private:
        static void udev_thread();

        static inline std::atomic_bool _thread_started = false;
        static inline std::atomic_bool _thread_stop = false;
        static inline std::thread _thread_inst;
        static inline std::recursive_mutex _instance_mutex;
        static inline std::atomic_int _instance_count = 0;
    };
}  // namespace scanbdpp
