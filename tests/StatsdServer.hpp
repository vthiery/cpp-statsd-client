#ifndef STATSD_SERVER_HPP
#define STATSD_SERVER_HPP

// It might make sense to include this test class in the UDPSender header
// it includes most of the cross platform defines etc that we need for socket io
#include "cpp-statsd-client/UDPSender.hpp"

#include <algorithm>
#include <string>

namespace Statsd {

class StatsdServer {
public:
    StatsdServer(unsigned short port = 8125) noexcept {
        // Create the fd
        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (!isValidSocket(m_socket)) {
            m_errorMessage = "socket creation failed: errno=" + std::to_string(errno);
            return;
        }

        // Binding should be with ipv4 to all interfaces
        struct sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = INADDR_ANY;

        // Try to bind
        if (bind(m_socket, reinterpret_cast<const struct sockaddr*>(&address), sizeof(address)) != 0) {
#ifdef _WIN32
            closesocket(m_socket);
#else
            close(m_socket);
#endif
            m_socket = k_invalidSocket;
            m_errorMessage = "Could not bind to address and port";
        }
    }

    ~StatsdServer() {
        if (isValidSocket(m_socket)) {
#ifdef _WIN32
            closesocket(m_socket);
#else
            close(m_socket);
#endif
        }
    }

    const std::string& errorMessage() const noexcept {
        return m_errorMessage;
    }

    std::string receive() noexcept {
        // If uninitialized then bail
        if (!isValidSocket(m_socket)) {
            return "";
        }

        // Try to receive (this is blocking)
        std::string buffer(256, '\0');
        int string_len;
        if ((string_len = recv(m_socket, &buffer[0], static_cast<int>(buffer.size()), 0)) < 1) {
            m_errorMessage = "Could not recv on the socket file descriptor";
            return "";
        }

        // No error return the trimmed result
        m_errorMessage.clear();
        buffer.resize(std::min(static_cast<size_t>(string_len), buffer.size()));
        return buffer;
    }

private:
    SOCKET_TYPE m_socket;
    std::string m_errorMessage;
};

}  // namespace Statsd

#endif
