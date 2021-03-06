#include "udev.h"
#include "common.h"

#include <chrono>
#include <thread>

#include "spdlog/spdlog.h"
#include "udevpp.h"

#include "device_events.h"
#include "sane.h"
#include "signal_handler.h"

namespace scanbdpp {
    UDevHandler::UDevHandler() {
        std::lock_guard<std::recursive_mutex> guard(_instance_mutex);
        ++_instance_count;
    }

    UDevHandler::~UDevHandler() {
        std::lock_guard<std::recursive_mutex> guard(_instance_mutex);
        --_instance_count;

        if (!_instance_count) {
            stop();
        }
    }

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
                                spdlog::get("logger")->info("Device added");
                                device_events.device_added();
                            } else if (device->get_action() == Constants::remove_action) {
                                device_events.device_removed();
                                spdlog::get("logger")->info("Device removed");
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
        std::lock_guard<std::recursive_mutex> guard(_instance_mutex);

        if (!_thread_started) {
            _thread_started = true;
            _thread_inst = std::thread(udev_thread);
            spdlog::get("logger")->info("Started udev thread");
        }
    }

    void UDevHandler::stop() const {
        std::lock_guard<std::recursive_mutex> guard(_instance_mutex);

        if (_thread_started) {
            _thread_stop = true;
            if (_thread_inst.joinable()) {
                _thread_inst.join();
                _thread_started = false;
                spdlog::get("logger")->info("Stopped udev thread");
            } else {
                spdlog::get("logger")->info("Couldn't join udev thread");
            }
        }
    }
}  // namespace scanbdpp
