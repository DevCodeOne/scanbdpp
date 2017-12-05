#include "udev.h"
#include "common.h"

#include <chrono>
#include <thread>

#include "udevpp.h"

#include "device_events.h"
#include "signal_handler.h"

namespace scanbdpp {
    void UDevHandler::udev_thread() {
        SignalHandler signal_handler;

        signal_handler.disable_signals_for_thread();

        udevpp::UDev udev;
        DeviceEvents device_events;

        if (udev) {
            udevpp::UDevMonitor device_monitor(udev, udevpp::UDevMonitor::Mode::NONBLOCKING);

            if (device_monitor) {
                while (!_thread_stop) {
                    auto device = device_monitor.receive_device();

                    if (device) {
                        if (device->get_device_type() == Constants::device_type) {
                            if (device->get_action() == Constants::add_action) {
                                device_events.device_added();
                            } else if (device->get_action() == Constants::remove_action) {
                                device_events.device_removed();
                            }
                        }
                    } else {
                        std::this_thread::sleep_for(Constants::no_device_sleep);
                    }
                }
            }
        }
    }

    void UDevHandler::start() const {
        if (!_thread_started) {
            _thread_started = true;
            _thread_inst = std::thread(udev_thread);
        }
    }

    void UDevHandler::stop() const {
        if (_thread_started) {
            _thread_stop = true;
        }

        if (_thread_inst.joinable()) {
            _thread_inst.join();
        }
    }
}  // namespace scanbdpp
