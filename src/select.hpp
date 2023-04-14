#pragma once

#include <sys/select.h>

#include "poller.hpp"

namespace NNet {

class TSelect: public TPollerBase {
    fd_set ReadFds_;
    fd_set WriteFds_;

public:
    TSelect() {
        FD_ZERO(&ReadFds_);
        FD_ZERO(&WriteFds_);
    }

    void Poll() {
        auto deadline = Timers_.empty() ? TTime::max() : Timers_.top().Deadline;
        auto tv = GetTimeval(TClock::now(), deadline);
        int maxFd = -1;

        FD_ZERO(&ReadFds_);
        FD_ZERO(&WriteFds_);

        for (auto& [k, ev] : Events_) {
            if (ev.Read) {
                FD_SET(k, &ReadFds_);
            }
            if (ev.Write) {
                FD_SET(k, &WriteFds_);
            }
            maxFd = std::max(maxFd, k);
        }
        if (select(maxFd+1, &ReadFds_, &WriteFds_, nullptr, &tv) < 0) { throw TSystemError(); }
        auto now = TClock::now();

        ReadyHandles_.clear();

        for (int k=0; k <= maxFd; ++k) {
            auto& ev = Events_[k];
            if (FD_ISSET(k, &WriteFds_)) {
                ReadyHandles_.emplace_back(std::move(ev.Write));
                ev.Write = {};
            }
            if (FD_ISSET(k, &ReadFds_)) {
                ReadyHandles_.emplace_back(std::move(ev.Read));
                ev.Read = {};
            }
        }

        while (!Timers_.empty()&&Timers_.top().Deadline <= now) {
            TTimer timer = std::move(Timers_.top());
            if (timer.Fd >= 0) {
                auto it = Events_.find(timer.Fd);
                if (it != Events_.end()) {
                    ReadyHandles_.emplace_back(it->second.Timeout);
                    it->second = {};
                }
            } else {
                ReadyHandles_.emplace_back(timer.Handle);
            }
            Timers_.pop();
        }
    }
};

} // namespace NNet
