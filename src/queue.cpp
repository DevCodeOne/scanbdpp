#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>

#include <cstring>

#include <chrono>
#include <thread>

#include "queue.h"

void QueueHandler::queue_thread() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_curmsgs = 0;
    attr.mq_maxmsg = _max_messages;
    attr.mq_msgsize = _max_message_size;

    int queue_des = mq_open(QueueHandler::queue_name, (O_RDONLY | O_CREAT | O_NONBLOCK), (S_IRUSR | S_IWUSR), &attr);

    if (queue_des < 0) {
        // Error opening queue
        return;
    }

    std::unique_ptr<char[]> message = std::make_unique<char[]>(_max_message_size);

    while (!_thread_stop) {
        if (int ret = mq_receive(queue_des, message.get(), _max_message_size, nullptr); ret < 0) {
            // Error receiving message
            // TODO Check errno and act accordingly
        } else {
            // TODO handle message
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    mq_close(queue_des);
    mq_unlink(QueueHandler::queue_name);
}

void QueueHandler::start() const {
    if (!_thread_started) {
        _thread_started = true;
        _thread_inst = std::thread(QueueHandler::queue_thread);
    }
}

void QueueHandler::stop() const {
    if (_thread_started) {
        _thread_stop = true;
    }

    if (_thread_inst.joinable()) {
        _thread_inst.join();
    }
}
