#ifndef UDP_SENDER_HPP
#define UDP_SENDER_HPP

#include <arpa/inet.h>
#include <cstring>
#include <deque>
#include <experimental/optional>
#include <cmath>
#include <mutex>
#include <netdb.h>
#include <string>
#include <thread>

#include "Socket.hpp"

namespace Statsd
{
/*!
 *
 * UDP sender
 *
 * A simple UDP sender handling batching.
 * Its configuration can be changed at runtime for
 * more flexibility.
 *
 */
class UDPSender final
{
public:

    //!@name Constructor and destructor
    //!@{

    //! Constructor
    UDPSender(
        const std::string& host,
        const uint16_t port,
        const std::experimental::optional<uint64_t> batchsize = std::experimental::nullopt) noexcept;

    //! Destructor
    ~UDPSender();

    //!@}

    //!@name Methods
    //!@{

    //! Sets a configuration { host, port }
    inline void setConfig(const std::string& host, const uint16_t port) noexcept;

    //! Send a message
    void send(const std::string& message) noexcept;

    //! Returns the error message as an optional string
    inline std::experimental::optional<std::string> errorMessage() const noexcept;

    //!@}

private:

    // @name Private methods
    // @{

    //! Initialize the sender and returns true when it is initialized
    bool initialize() noexcept;

    //! Send a message to the daemon
    void sendToDaemon(const std::string& message) noexcept;

    //!@}

private:

    // @name State variables
    // @{

    //! Is the sender initialized?
    bool m_isInitialized{ false };

    //! Shall we exit?
    bool m_mustExit{ false };

    //!@}

    // @name Network info
    // @{

    //! The hostname
    std::string m_host;

    //! The port
    uint16_t m_port;

    //! The structure holding the server
    struct sockaddr_in m_server;

    //! The socket to be used
    Socket m_socket;

    //!@}

    // @name Batching info
    // @{

    //! Shall the sender use batching?
    bool m_batching{ false };

    //! The batching size
    uint64_t m_batchsize;

    //! The queue batching the messages
    mutable std::deque<std::string> m_batchingMessageQueue;

    //! The mutex used for batching
    std::mutex m_batchingMutex;

    //! The thread dedicated to the batching
    std::thread m_batchingThread;

    //!@}

    //! Error message (optional string)
    std::experimental::optional<std::string> m_errorMessage;
};

UDPSender::
UDPSender(
    const std::string& host,
    const uint16_t port,
    const std::experimental::optional<uint64_t> batchsize) noexcept
: m_host(host)
, m_port(port)
{
    // If batching is on, use a dedicated thread to send every now and then
    if (batchsize)
    {
        // Thread' sleep duration between batches
        // TODO: allow to input this
        constexpr unsigned int batchingWait{ 1000U };

        m_batching = true;
        m_batchsize = batchsize.value();

        // Define the batching thread
        m_batchingThread = std::thread([this] {
            while (!m_mustExit)
            {
                std::deque<std::string> stagedMessageQueue;

                std::unique_lock<std::mutex> batchingLock(m_batchingMutex);
                m_batchingMessageQueue.swap(stagedMessageQueue);
                batchingLock.unlock();

                // Flush the queue
                while (!stagedMessageQueue.empty())
                {
                    sendToDaemon(stagedMessageQueue.front());
                    stagedMessageQueue.pop_front();
                }

                // Wait before sending the next batch
                std::this_thread::sleep_for(std::chrono::milliseconds(batchingWait));
            }
        });
    }
}

UDPSender::
~UDPSender()
{
    if (m_batching)
    {
        m_mustExit = true;
        m_batchingThread.join();
    }

    m_socket.close();
}

void
UDPSender::
setConfig(const std::string& host, const uint16_t port) noexcept
{
    m_host = host;
    m_port = port;

    m_isInitialized = false;

    m_socket.close();
}

void
UDPSender::
send(const std::string& message) noexcept
{
    // If batching is on, accumulate messages in the queue
    if (m_batching)
    {
        std::unique_lock<std::mutex> batchingLock(m_batchingMutex);
        if (m_batchingMessageQueue.empty() || m_batchingMessageQueue.back().length() > m_batchsize)
        {
            m_batchingMessageQueue.push_back(message);
        }
        else
        {
            std::rbegin(m_batchingMessageQueue)->append("\n").append(message);
        }
    }
    else
    {
        sendToDaemon(message);
    }
}

std::experimental::optional<std::string>
UDPSender::
errorMessage() const noexcept
{
    return m_errorMessage;
}

bool
UDPSender::
initialize() noexcept
{
    if (m_isInitialized)
    {
        return true;
    }

    // Connect the socket
    if (!m_socket.connect(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Could not create socket, err=%m");
        m_errorMessage = std::string(buffer);
        return false;
    }

    std::memset(&m_server, 0, sizeof(m_server));
    m_server.sin_family = AF_INET;
    m_server.sin_port = htons(m_port);

    if (inet_aton(m_host.c_str(), &m_server.sin_addr) == 0)
    {
        // An error code has been returned by inet_aton

        // Specify the criteria for selecting the socket address structure
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        // Get the address info using the hints
        struct addrinfo* results = nullptr;
        const int ret = getaddrinfo(m_host.c_str(), nullptr, &hints, &results);
        if (ret != 0)
        {
            // An error code has been returned by getaddrinfo
            m_socket.close();

            char buffer[256];
            snprintf(buffer, sizeof(buffer), "getaddrinfo failed: error=%d, msg=%s", ret, gai_strerror(ret));
            m_errorMessage = std::string(buffer);

            return false;
        }

        // Copy the results in m_server
        struct sockaddr_in* host_addr = (struct sockaddr_in*)results->ai_addr;
        std::memcpy(&m_server.sin_addr, &host_addr->sin_addr, sizeof(struct in_addr));

        // Free the memory allocated
        freeaddrinfo(results);
    }

    m_isInitialized = true;

    return true;
}

void
UDPSender::
sendToDaemon(const std::string& message) noexcept
{
    // Can't send until the sender is initialized
    if (!initialize())
    {
        return;
    }

    // Try sending the message
    if (!m_socket.send(message, m_server))
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "sendto server failed: host=%s:%d, err=%m", m_host.c_str(), m_port);
        m_errorMessage = std::string(buffer);
    }
}

}

#endif
