#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdint>
#include <cstring>

#include <chrono>
#include <string>
#include <thread>

#include "spdlog/spdlog.h"

#include "pipe.h"
#include "sane.h"
#include "signal_handler.h"

namespace scanbdpp {

    PipeHandler::PipeHandler() {
        std::lock_guard<std::mutex> guard{_instance_mutex};

        ++_instance_count;
    }

    PipeHandler::~PipeHandler() {
        std::lock_guard<std::mutex> guard{_instance_mutex};

        --_instance_count;

        if (!_instance_count) {
            stop();
        }
    }

    void PipeHandler::pipe_thread() {
        SignalHandler signal_handler;
        signal_handler.disable_signals_for_thread();
        SaneHandler handler;

        if (mkfifo(PipeHandler::pipe_path.c_str(), S_IRUSR | S_IWUSR) != 0) {
            spdlog::get("logger")->warn("Error creating pipe {0}", strerror(errno));
        }

        int pipe_des = open(PipeHandler::pipe_path.c_str(), O_RDONLY | O_NONBLOCK);

        if (pipe_des < 0) {
            spdlog::get("logger")->critical("Error opening pipe {0}", strerror(errno));
            return;
        }

        char buf[_max_message_size];

        while (!_thread_stop) {
            if (int ret = read(pipe_des, buf, _max_message_size); ret < 0) {
                switch (errno) {
                    case EAGAIN:
                        break;
                    default:
                        spdlog::get("logger")->critical(strerror(errno));
                        _thread_stop = true;
                }

            } else if (ret != 0) {
                std::istringstream message(buf);
                std::string device;
                std::string action;
                if (std::getline(message, device, ',') && std::getline(message, action, ',')) {
                    spdlog::get("logger")->info("Received message to trigger action {0} on device {1}", action, device);
                    handler.trigger_action(device, action);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        close(pipe_des);
        unlink(PipeHandler::pipe_path.c_str());
    }

    void PipeHandler::start() const {
        if (!_thread_started) {
            _thread_started = true;
            _thread_inst = std::thread(PipeHandler::pipe_thread);
            spdlog::get("logger")->info("Starting pipe thread");
        }
    }

    void PipeHandler::stop() const {
        if (_thread_started) {
            _thread_stop = true;
        } else {
            return;
        }

        if (_thread_inst.joinable()) {
            _thread_inst.join();
            _thread_started = false;
            spdlog::get("logger")->info("Stopped pipe thread");
        } else {
            spdlog::get("logger")->info("Couldn't join pipe handler");
        }
    }

    void PipeHandler::write_message(const std::string &message) const {
        int pipe_des = open(PipeHandler::pipe_path.c_str(), O_WRONLY | O_NONBLOCK);

        if (pipe_des < 0) {
            spdlog::get("logger")->critical("Error opening pipe {0}", strerror(errno));
            return;
        }

        if (message.size() > PipeHandler::_max_message_size) {
            spdlog::get("logger")->critical("Message is too long");
            return;
        }

        // Write everything in one buffer with a size of _max_message_size := PIPE_BUF
        // With this, the write is guaranteed to be atomic
        // PIPE_BUF is at least 512
        int written = write(pipe_des, (const void *)message.c_str(), message.size() + 1);

        if (written < 0) {
            spdlog::get("logger")->critical("Error writing message {0}", strerror(errno));
        } else if ((size_t)written != message.size() + 1) {
            spdlog::get("logger")->critical("Writing was not atomic, shouldn't happen");
        }
    }
}  // namespace scanbdpp
