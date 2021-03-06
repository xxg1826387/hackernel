/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef HACKERNEL_BROADCASTER_H
#define HACKERNEL_BROADCASTER_H

#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <queue>

namespace hackernel {

int stop_all_audience();

class broadcaster;
class audience;

class audience {
public:
    void set_broadcaster(std::weak_ptr<broadcaster> broadcaster);
    void save_message(std::string message);
    void start_consuming_message();
    void add_message_handler(std::function<bool(const std::string &)> new_handler);
    void stop_consuming_message();

private:
    int wait_message(std::string &message);

private:
    std::weak_ptr<broadcaster> bind_broadcaster_;
    std::queue<std::string> message_queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_ = false;
    std::list<std::function<bool(const std::string &)>> handlers_;
};

class broadcaster : public std::enable_shared_from_this<broadcaster> {
public:
    static broadcaster &global();
    void add_audience(std::shared_ptr<audience> audience);
    void del_audience(std::shared_ptr<audience> audience);
    void broadcast(std::string message);
    void notify_audience_stop();

private:
    std::list<std::shared_ptr<audience>> audience_;
    std::mutex mutex_;
};

}; // namespace hackernel

#endif
