#ifndef STATSD_SERVER_HPP
#define STATSD_SERVER_HPP

// TODO: should we actually add this to the distributed headers? With a tiny bit of work
//  someone could write their own c++ statsd server


class StatsdServer {
public:
    StatsdServer(unsigned short port = 8125) noexcept {
        // Create the fd
        if((m_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            m_errorMessage = "Could not create socket file descriptor";
            return;
        }

        // Binding should be with ipv4 to all interfaces
        struct sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = INADDR_ANY;

        // Try to bind
        if(bind(m_fd, reinterpret_cast<const struct sockaddr*>(&address), sizeof(address)) != 0) {
            m_fd = -1;
            m_errorMessage = "Could not bind to address and port";
            return;
        }
    }

    const std::string& errorMessage() const noexcept {
        return m_errorMessage;
    }

    std::string receive() noexcept {
        // If uninitialized then bail
        if(m_fd == -1) {
            return "";
        }

        // Try to receive (this is blocking)
        std::string buffer(256, '\0');
        int string_len = -1;
        if((string_len = recv(m_fd, &buffer[0], buffer.size(), 0)) < 1) {
            m_errorMessage = "Could not recv on the socket file descriptor";
            return "";
        }

        // No error return the trimmed result
        m_errorMessage.clear();
        buffer.resize(std::min(string_len, static_cast<int>(buffer.size())));
        return buffer;
    }

private:
    int m_fd;
    std::string m_errorMessage;
};

#endif
