#ifndef UDP_SENDER_HPP
#define UDP_SENDER_HPP

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace Statsd {
/*!
 *
 * UDP sender
 *
 * A simple UDP sender handling batching.
 *
 */
class UDPSender final {
public:
    //!@name Constructor and destructor
    //!@{

    //! Constructor
    UDPSender(const std::string& host, const uint16_t port, const uint64_t batchsize = 0) noexcept;

    //! Destructor
    ~UDPSender();

    //!@}

    //!@name Methods
    //!@{

    //! Send a message
    void send(const std::string::const_iterator& begin, const std::string::const_iterator& end) noexcept;

    //! Returns the error message as a string
    const std::string& errorMessage() const noexcept;

    //! Set the error message
    void setErrorMessage(const std::string& error) noexcept;

    //! Returns true if the sender is initialized
    bool initialized() const noexcept;

    //!@}

private:
    // @name Private methods
    // @{

    //! Initialize the sender and returns true when it is initialized
    bool initialize() noexcept;

    //! Send a message to the daemon
    void sendToDaemon(const std::string::const_iterator& begin, const std::string::const_iterator& end) noexcept;

    //!@}

private:
    // @name State variables
    // @{

    //! Shall we exit?
    std::atomic<bool> m_mustExit{false};

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
    int m_socket{-1};

    //!@}

    // @name Batching info
    // @{

    //! Shall the sender use batching?
    bool m_batching{false};

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
    std::string m_errorMessage;

    //! Bad file descriptor
    static constexpr int k_invalidFd{-1};
};

inline UDPSender::UDPSender(const std::string& host, const uint16_t port, const uint64_t batchsize) noexcept
    : m_host(host), m_port(port) {
    // Initialize the socket
    if (!initialize()) {
        return;
    }

    // If batching is on, use a dedicated thread to send after the wait time is reached
    if (batchsize != 0) {
        // Thread' sleep duration between batches
        // TODO: allow to input this
        constexpr unsigned int batchingWait{1000U};

        m_batching = true;
        m_batchsize = batchsize;

        // Define the batching thread
        m_batchingThread = std::thread([this, batchingWait] {
            // TODO: this will drop unsent stats, should we send all the unsent stats before we exit?
            while (!m_mustExit.load(std::memory_order_acq_rel)) {
                std::deque<std::string> stagedMessageQueue;

                std::unique_lock<std::mutex> batchingLock(m_batchingMutex);
                m_batchingMessageQueue.swap(stagedMessageQueue);
                batchingLock.unlock();

                // Flush the queue
                while (!stagedMessageQueue.empty()) {
                    sendToDaemon(stagedMessageQueue.front().cbegin(), stagedMessageQueue.front().cend());
                    stagedMessageQueue.pop_front();
                }

                // Wait before sending the next batch
                std::this_thread::sleep_for(std::chrono::milliseconds(batchingWait));
            }
        });
    }
}

inline UDPSender::~UDPSender() {
    if (!initialized()) {
        return;
    }

    if (m_batching) {
        m_mustExit.store(true, std::memory_order_acq_rel);
        m_batchingThread.join();
    }

    close(m_socket);
}

inline void UDPSender::send(const std::string::const_iterator& begin, const std::string::const_iterator& end) noexcept {
    m_errorMessage.clear();
    // If batching is on, accumulate messages in the queue
    if (m_batching) {
        std::unique_lock<std::mutex> batchingLock(m_batchingMutex);
        // Either we don't have a place to batch our message or we exceeded the batch size, so make a new batch
        if (m_batchingMessageQueue.empty() || m_batchingMessageQueue.back().length() > m_batchsize) {
            m_batchingMessageQueue.emplace_back();
            m_batchingMessageQueue.back().reserve(m_batchsize + 256);
        }  // When there is already a batch open we need a separator when its not empty
        else if (!m_batchingMessageQueue.back().empty()) {
            m_batchingMessageQueue.back().push_back('\n');
        }
        // Add the new message to the batch
        m_batchingMessageQueue.back().append(begin, end);
    } else {
        sendToDaemon(begin, end);
    }
}

inline const std::string& UDPSender::errorMessage() const noexcept {
    return m_errorMessage;
}

inline void UDPSender::setErrorMessage(const std::string& error) noexcept {
    m_errorMessage = error;
}

inline bool UDPSender::initialize() noexcept {
    // Connect the socket
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == k_invalidFd) {
        m_errorMessage = std::string("socket creation failed: err=") + std::strerror(errno);
        return false;
    }

    std::memset(&m_server, 0, sizeof(m_server));
    m_server.sin_family = AF_INET;
    m_server.sin_port = htons(m_port);

    if (inet_aton(m_host.c_str(), &m_server.sin_addr) == 0) {
        // An error code has been returned by inet_aton

        // Specify the criteria for selecting the socket address structure
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        // Get the address info using the hints
        struct addrinfo* results = nullptr;
        const int ret{getaddrinfo(m_host.c_str(), nullptr, &hints, &results)};
        if (ret != 0) {
            // An error code has been returned by getaddrinfo
            close(m_socket);
            m_socket = k_invalidFd;
            m_errorMessage = "getaddrinfo failed: err=" + std::to_string(ret) + ", msg=" + gai_strerror(ret);
            return false;
        }

        // Copy the results in m_server
        struct sockaddr_in* host_addr = (struct sockaddr_in*)results->ai_addr;
        std::memcpy(&m_server.sin_addr, &host_addr->sin_addr, sizeof(struct in_addr));

        // Free the memory allocated
        freeaddrinfo(results);
    }

    return true;
}

inline void UDPSender::sendToDaemon(const std::string::const_iterator& begin,
                                    const std::string::const_iterator& end) noexcept {
    // Try sending the message
    const long int ret{sendto(m_socket, &*begin, end - begin, 0, (struct sockaddr*)&m_server, sizeof(m_server))};
    if (ret == -1) {
        m_errorMessage =
            "sendto server failed: host=" + m_host + ":" + std::to_string(m_port) + ", err=" + std::strerror(errno);
    }
}

inline bool UDPSender::initialized() const noexcept {
    return m_socket != k_invalidFd;
}

}  // namespace Statsd

#endif
