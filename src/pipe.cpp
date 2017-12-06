#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdint>
#include <cstring>

#include <chrono>
#include <thread>

#include "spdlog/spdlog.h"

#include "pipe.h"
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

        if (mkfifo(PipeHandler::pipe_path.c_str(), S_IRUSR | S_IWUSR) != 0) {
            spdlog::get("logger")->warn("Error creating pipe {0}", strerror(errno));
        }

        int pipe_des = open(PipeHandler::pipe_path.c_str(), O_RDONLY | O_NONBLOCK);

        if (pipe_des < 0) {
            spdlog::get("logger")->critical("Error opening pipe {0}", strerror(errno));
            return;
        }

        std::unique_ptr<char[]> message = std::make_unique<char[]>(_max_message_size);

        while (!_thread_stop) {
            if (int ret = read(pipe_des, message.get(), _max_message_size); ret < 0) {
                // Error receiving message
                // TODO Check errno and act accordingly
                switch (errno) {
                    case EAGAIN:
                        break;
                    default:
                        spdlog::get("logger")->critical(strerror(errno));
                        _thread_stop = true;
                }

            } else if (ret != 0) {
                // TODO handle message
                spdlog::get("logger")->info(message.get() + sizeof(size_t));
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
        }
    }

    void PipeHandler::stop() const {
        if (_thread_started) {
            _thread_stop = true;
        }

        if (_thread_inst.joinable()) {
            _thread_inst.join();
        }
    }

    void PipeHandler::write_message(const std::string &message) const {
        int pipe_des = open(PipeHandler::pipe_path.c_str(), O_WRONLY | O_NONBLOCK);

        if (pipe_des < 0) {
            spdlog::get("logger")->critical("Error opening pipe {0}", strerror(errno));
            return;
        }

        if (message.size() + sizeof(size_t) > PipeHandler::_max_message_size) {
            spdlog::get("logger")->critical("Message is too long");
        }

        // Write everything in one buffer with a size of _max_message_size := PIPE_BUF
        // With this, the write is guaranteed to be atomic
        // PIPE_BUF is at least 512
        unsigned char buffer[PipeHandler::_max_message_size];
        size_t message_size = message.size() + sizeof(size_t);

        std::memcpy((void *)buffer, (const void *)&message_size, sizeof(size_t));
        std::memcpy((void *)(buffer + sizeof(size_t)), (const void *)message.c_str(), message.size() + 1);

        int written = write(pipe_des, (const void *)buffer, message_size);

        if (written < 0) {
            spdlog::get("logger")->critical("Error writing message {0}", strerror(errno));
        } else if ((size_t)written != message_size) {
            spdlog::get("logger")->critical("Writing was not atomic, shouldn't happen");
        }
    }
}  // namespace scanbdpp
