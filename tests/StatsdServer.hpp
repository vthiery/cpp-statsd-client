#ifndef STATSD_SERVER_HPP
#define STATSD_SERVER_HPP

#ifdef _WIN32
#define NOMINMAX

#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>

using SOCKET_TYPE = SOCKET;
constexpr SOCKET_TYPE k_invalidFd{INVALID_SOCKET};
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using SOCKET_TYPE = int;
constexpr SOCKET_TYPE k_invalidFd{-1};
#endif

#include <algorithm>
#include <string>

namespace Statsd {

class StatsdServer {
public:
    StatsdServer(unsigned short port = 8125) noexcept {
        // Create the fd
        if (!isValidFd(m_fd = socket(AF_INET, SOCK_DGRAM, 0))) {
            m_errorMessage = "Could not create socket file descriptor";
            return;
        }

        // Binding should be with ipv4 to all interfaces
        struct sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = INADDR_ANY;

        // Try to bind
        if (bind(m_fd, reinterpret_cast<const struct sockaddr*>(&address), sizeof(address)) != 0) {
#ifdef _WIN32
            closesocket(m_fd);
#else
            close(m_fd);
#endif
            m_fd = k_invalidFd;
            m_errorMessage = "Could not bind to address and port";
        }
    }

    ~StatsdServer() {
        if (isValidFd(m_fd)) {
#ifdef _WIN32
            closesocket(m_fd);
#else
            close(m_fd);
#endif
        }
    }

    const std::string& errorMessage() const noexcept {
        return m_errorMessage;
    }

    std::string receive() noexcept {
        // If uninitialized then bail
        if (!isValidFd(m_fd)) {
            return "";
        }

        // Try to receive (this is blocking)
        std::string buffer(256, '\0');
        int string_len;
        if ((string_len = recv(m_fd, &buffer[0], static_cast<int>(buffer.size()), 0)) < 1) {
            m_errorMessage = "Could not recv on the socket file descriptor";
            return "";
        }

        // No error return the trimmed result
        m_errorMessage.clear();
        buffer.resize(std::min(string_len, static_cast<int>(buffer.size())));
        return buffer;
    }

private:
    static inline bool isValidFd(const SOCKET_TYPE fd) {
        return fd != k_invalidFd;
    }

    SOCKET_TYPE m_fd;
    std::string m_errorMessage;
};

}  // namespace Statsd

#endif
