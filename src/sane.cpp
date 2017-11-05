#include "common.h"

#include "sane.h"

#include <atomic>
#include <chrono>
#include <thread>

#include "sanepp.h"

#include "config.h"

namespace scanbdpp {
    void SaneHandler::poll_device(sanepp::Sane instance, sanepp::DeviceInfo device_info) {
        auto sane_instance(instance);
        auto device = device_info.open();

        if (device) {
        } else {
            // Error couldn't open device
        }
    }

    void SaneHandler::start() {
        if (_device_threads.size() != 0) {
            stop();
        }

        // critical section
        {
            sanepp::Sane sane_instance;
            std::lock_guard<std::mutex> device_guard(_device_mutex);
            for (auto device_info : sane_instance.devices(false)) {
                _device_threads.emplace_back(poll_device, sane_instance, device_info);
            }
        }
    }

    void SaneHandler::stop() {
        if (_device_threads.size() == 0) {
            return;
        }

        // critical sections
        {
            std::lock_guard<std::mutex> device_guard(_device_mutex);
            _terminate_threads = true;
            for (auto &current_thread : _device_threads) {
                // stopping poll thread for device
                if (current_thread.joinable()) {
                    current_thread.join();
                }
            }

            _device_threads.clear();
        }
    }
}  // namespace scanbdpp
