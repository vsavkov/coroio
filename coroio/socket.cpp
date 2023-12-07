#include "socket.hpp"

namespace NNet {

TAddress::TAddress(const std::string& addr, int port)
{
    inet_pton(AF_INET, addr.c_str(), &(Addr_.sin_addr));
    Addr_.sin_port = htons(port);
    Addr_.sin_family = AF_INET;
}

TAddress::TAddress(sockaddr_in addr)
    : Addr_(addr)
{ }

sockaddr_in TAddress::Addr() const { return Addr_; }

bool TAddress::operator == (const TAddress& other) const {
    return memcmp(&Addr_, &other.Addr_, sizeof(Addr_)) == 0;
}

TSocketOps::TSocketOps(TPollerBase& poller)
    : Poller_(&poller)
    , Fd_(Create())
{ }

TSocketOps::TSocketOps(int fd, TPollerBase& poller)
    : Poller_(&poller)
    , Fd_(Setup(fd))
{ }

int TSocketOps::Create() {
    auto s = socket(PF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        throw std::system_error(errno, std::generic_category(), "socket");
    }
    return Setup(s);
}

int TSocketOps::Setup(int s) {
    struct stat statbuf;
    fstat(s, &statbuf);
    if (S_ISSOCK(statbuf.st_mode)) {
        int value;
        value = 1;
        if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(int)) < 0) {
            throw std::system_error(errno, std::generic_category(), "setsockopt");
        }
        value = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) < 0) {
            throw std::system_error(errno, std::generic_category(), "setsockopt");
        }
    }
    auto flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) {
        throw std::system_error(errno, std::generic_category(), "fcntl");
    }
    if (fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::system_error(errno, std::generic_category(), "fcntl");
    }
    return s;
}

TSocket::TSocket(TAddress&& addr, TPollerBase& poller)
    : TSocketBase(poller)
    , Addr_(std::move(addr))
{ }

TSocket::TSocket(const TAddress& addr, int fd, TPollerBase& poller)
    : TSocketBase(fd, poller)
    , Addr_(addr)
{ }

TSocket::TSocket(const TAddress& addr, TPollerBase& poller)
    : TSocketBase(poller)
    , Addr_(addr)
{ }

TSocket::TSocket(TSocket&& other)
{
    *this = std::move(other);
}

TSocket::~TSocket()
{
    if (Fd_ >= 0) {
        close(Fd_);
        Poller_->RemoveEvent(Fd_);
    }
}

TSocket& TSocket::operator=(TSocket&& other) {
    if (this != &other) {
        Poller_ = other.Poller_;
        Addr_ = other.Addr_;
        Fd_ = other.Fd_;
        other.Fd_ = -1;
    }
    return *this;
}

void TSocket::Bind() {
    auto addr = Addr_.Addr();
    if (bind(Fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::system_error(errno, std::generic_category(), "bind");
    }
}

void TSocket::Listen(int backlog) {
    if (listen(Fd_, backlog) < 0) {
        throw std::system_error(errno, std::generic_category(), "listen");
    }
}

const TAddress& TSocket::Addr() const {
    return Addr_;
}

int TSocket::Fd() const {
    return Fd_;
}

} // namespace NNet {