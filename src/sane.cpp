#include "common.h"

#include <atomic>
#include <chrono>
#include <thread>

#include "sanepp.h"

#include "config.h"
#include "sane.h"

namespace scanbdpp {
    static std::mutex device_mutex;
    static std::vector<std::thread> device_threads;
    static std::atomic_bool terminate_threads = false;

    void poll_device(sanepp::Sane instance, sanepp::DeviceInfo device_info) {
        auto sane_instance(instance);
        auto device = device_info.open();

        if (device) {
        } else {
            // Error couldn't open device
        }
    }

    void start_sane_threads() {
        if (device_threads.size() != 0) {
            stop_sane_threads();
        }

        // critical section
        {
            sanepp::Sane sane_instance;
            std::lock_guard<std::mutex> device_guard(device_mutex);
            for (auto device_info : sane_instance.devices(false)) {
                device_threads.emplace_back(poll_device, sane_instance, device_info);
            }
        }
    }

    void stop_sane_threads() {
        if (device_threads.size() == 0) {
            return;
        }

        // critical sections
        {
            std::lock_guard<std::mutex> device_guard(device_mutex);
            terminate_threads = true;
            for (auto &current_thread : device_threads) {
                // stopping poll thread for device
                if (current_thread.joinable()) {
                    current_thread.join();
                }
            }

            device_threads.clear();
        }
    }
}  // namespace scanbdpp
