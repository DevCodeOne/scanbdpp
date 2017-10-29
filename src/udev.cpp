#include "common.h"

#include <atomic>
#include <chrono>
#include <thread>

#include "udevpp.h"

#include "udev.h"
#include "device_events.h"

static constexpr char add_action[] = "add";
static constexpr char remove_action[] = "remove";
static constexpr char device_type[] = "usb_device";
static constexpr std::chrono::duration no_device_sleep = std::chrono::seconds(1);
static bool udev_thread_started(false);
static std::atomic_bool udev_thread_stop(false);
static std::thread udev_thread_inst;

namespace scanbdpp {
    static void udev_thread() {
        udevpp::UDev udev;

        if (udev) {
            udevpp::UDevMonitor device_monitor(udev, udevpp::UDevMonitor::Mode::NONBLOCKING);

            if (device_monitor) {
                while (!udev_thread_stop) {
                    auto device = device_monitor.receive_device();

                    if (device) {
                        if (device->get_device_type() == device_type) {
                            if (device->get_action() == add_action) {
                                signal_device_added();
                            } else if (device->get_action() == remove_action) {
                                signal_device_removed();
                            }
                        }
                    } else {
                        std::this_thread::sleep_for(no_device_sleep);
                    }
                }
            }
        }
    }

    void start_udev_thread() {
        if (!udev_thread_started) {
            udev_thread_started = true;
            udev_thread_inst = std::thread(udev_thread);
        }
    }

    void stop_udev_thread() {
        if (udev_thread_started) {
            udev_thread_stop = true;
        }

        udev_thread_inst.join();
    }
}  // namespace scanbdpp
