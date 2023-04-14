#pragma once

#include <vector>
#include <unordered_map>
#include <queue>

#include "base.hpp"

namespace NNet {

class TPollerBase {
public:
    void AddTimer(int fd, TTime deadline, THandle h) {
        Timers_.emplace(TTimer{deadline, fd, h});
        if (fd >= 0) {
            Events_[fd].Timeout = std::move(h);
        }
    }

    bool RemoveTimer(int fd) {
        bool r = !!Events_[fd].Timeout;
        Events_[fd].Timeout = {};
        return r;
    }

    void AddRead(int fd, THandle h) {
        Events_[fd].Read = std::move(h);
    }

    void AddWrite(int fd, THandle h) {
        Events_[fd].Write = std::move(h);
    }

    void RemoveEvent(int fd) {
        Events_.erase(fd);
    }

    template<typename Rep, typename Period>
    auto Sleep(std::chrono::duration<Rep,Period> duration) {
        auto now = TClock::now();
        auto next= now+duration;
        struct TAwaitable {
            bool await_ready() {
                return false;
            }

            void await_suspend(std::coroutine_handle<> h) {
                poller->AddTimer(-1, n, h);
            }

            void await_resume() { }

            TPollerBase* poller;
            TTime n;
        };
        return TAwaitable{this,next};
    }

    auto& ReadyHandles() {
        return ReadyHandles_;
    }

protected:
    std::unordered_map<int,TEvent> Events_;
    std::priority_queue<TTimer> Timers_;
    std::vector<THandle> ReadyHandles_;
};

} // namespace NNet
