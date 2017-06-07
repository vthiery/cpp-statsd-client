#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace Statsd
{
/*!
 *
 * Socket
 *
 * A simple socker class.
 *
 */
class Socket
{
public:

    //!@name Destructor
    //!@{

    //! Destructor
    ~Socket();

    //!@}

    //!@name Methods
    //!@{

    //! Connects the socket
    inline bool connect(const int domain, const int type, const int protocol) noexcept;

    //! Closes the socket
    inline void close() noexcept;

    //! Sends a message to the destination
    inline bool send(const std::string& message, struct sockaddr_in destination) const noexcept;

    //!@}

private:

    //! File descriptor received from connect
    int m_fileDescriptor{ -1 };

    //! Is it connected or closed?
    bool m_isConnected{ false };
};

Socket::
~Socket()
{
    close();
}

bool
Socket::
connect(const int domain, const int type, const int protocol) noexcept
{
    m_fileDescriptor = socket(domain, type, protocol);
    m_isConnected = (m_fileDescriptor != 1);
    return m_isConnected;
}

void
Socket::
close() noexcept
{
    if (m_isConnected)
    {
        m_isConnected = false;
        ::close(m_fileDescriptor);
    }
}

bool
Socket::
send(const std::string& message, struct sockaddr_in destination) const noexcept
{
    return sendto(m_fileDescriptor, message.data(), message.size(), 0, (struct sockaddr *)&destination, sizeof(destination)) != -1;
}

}

#endif
