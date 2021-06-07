#ifndef STATSD_CLIENT_HPP
#define STATSD_CLIENT_HPP

#include <cstdio>
#include <memory>
#include <random>
#include <string>
#include "UDPSender.hpp"

namespace Statsd {
/*!
 *
 * Statsd client
 *
 * This is the Statsd client, exposing the classic methods
 * and relying on a UDP sender for the actual sending.
 *
 * The sampling frequency can be input, as well as the
 * batching size. The latter is optional and shall be
 * set to 0 if batching is not desired.
 *
 */
class StatsdClient {
public:
    //!@name Constructor and destructor
    //!@{

    //! Constructor
    StatsdClient(const std::string& host,
                 const uint16_t port,
                 const std::string& prefix,
                 const uint64_t batchsize = 0) noexcept;

    StatsdClient(const StatsdClient&) = delete;
    StatsdClient& operator=(const StatsdClient&) = delete;

    //!@}

    //!@name Methods
    //!@{

    //! Sets a configuration { host, port, prefix, batchsize }
    void setConfig(const std::string& host, const uint16_t port, const std::string& prefix, const uint64_t batchsize = 0) noexcept;

    //! Returns the error message as an std::string
    const std::string& errorMessage() const noexcept;

    //! Increments the key, at a given frequency rate
    void increment(const std::string& key, const float frequency = 1.0f) const noexcept;

    //! Increments the key, at a given frequency rate
    void decrement(const std::string& key, const float frequency = 1.0f) const noexcept;

    //! Adjusts the specified key by a given delta, at a given frequency rate
    void count(const std::string& key, const int delta, const float frequency = 1.0f) const noexcept;

    //! Records a gauge for the key, with a given value, at a given frequency rate
    void gauge(const std::string& key, const unsigned int value, const float frequency = 1.0f) const noexcept;

    //! Records a timing for a key, at a given frequency
    void timing(const std::string& key, const unsigned int ms, const float frequency = 1.0f) const noexcept;

    //! Send a value for a key, according to its type, at a given frequency
    void send(const std::string& key, const int value, const std::string& type, const float frequency = 1.0f) const
        noexcept;

    //! Seed the RNG that controls sampling
    static void seed(unsigned int seed = std::random_device()()) noexcept;

    //!@}

private:
    //! The prefix to be used for metrics
    std::string m_prefix;

    //! The UDP sender to be used for actual sending
    std::unique_ptr<UDPSender> m_sender;

    //! The random number generator for handling sampling
    static thread_local std::mt19937 m_random_engine;
};

thread_local std::mt19937 StatsdClient::m_random_engine;

inline StatsdClient::StatsdClient(const std::string& host,
                                  const uint16_t port,
                                  const std::string& prefix,
                                  const uint64_t batchsize) noexcept
    : m_prefix(prefix), m_sender(new UDPSender{host, port, batchsize}) {
    // TODO: the final metric strings do not automatically place a '.' between the prefix and the key
    //  This might be unexpected to end users. We should either document this or we could just add the
    //  automatic '.' when the prefix is non empty
    //if(!m_prefix.empty()) {
    //    m_prefix.push_back('.');
    //}

    // Initialize the random generator to be used for sampling
    seed();
}

inline void StatsdClient::setConfig(const std::string& host, const uint16_t port, const std::string& prefix, const uint64_t batchsize) noexcept {
    m_prefix = prefix;
    m_sender.reset(new UDPSender(host, port, batchsize));
}

inline const std::string& StatsdClient::errorMessage() const noexcept {
    return m_sender->errorMessage();
}

inline void StatsdClient::decrement(const std::string& key, const float frequency) const noexcept {
    return count(key, -1, frequency);
}

inline void StatsdClient::increment(const std::string& key, const float frequency) const noexcept {
    return count(key, 1, frequency);
}

inline void StatsdClient::count(const std::string& key, const int delta, const float frequency) const noexcept {
    return send(key, delta, "c", frequency);
}

inline void StatsdClient::gauge(const std::string& key, const unsigned int value, const float frequency) const
    noexcept {
    return send(key, value, "g", frequency);
}

inline void StatsdClient::timing(const std::string& key, const unsigned int ms, const float frequency) const noexcept {
    return send(key, ms, "ms", frequency);
}

inline void StatsdClient::send(const std::string& key,
                               const int value,
                               const std::string& type,
                               const float frequency) const noexcept {
    constexpr float epsilon{0.0001f};
    const bool isFrequencyOne = std::fabs(frequency - 1.0f) < epsilon;

    // If you are sampling at a rate less than 1 (ie not sending every metric) and the RNG is above the sampling rate
    // then we don't need to send this metric this time
    if (!isFrequencyOne && (frequency < std::uniform_real_distribution<float>(0.f, 1.f)(m_random_engine))) {
        return;
    }

    // Prepare the buffer and include the sampling rate if it's not 1.f
    std::string buffer(256, '\0');
    int string_len = -1;
    if (isFrequencyOne) {
        // Sampling rate is 1.0f, no need to specify it
        string_len = std::snprintf(
            &buffer.front(), buffer.size(), "%s%s:%d|%s", m_prefix.c_str(), key.c_str(), value, type.c_str());
    } else {
        // Sampling rate is different from 1.0f, hence specify it
        string_len = std::snprintf(&buffer.front(),
                                          buffer.size(),
                                          "%s%s:%d|%s|@%.2f",
                                          m_prefix.c_str(),
                                          key.c_str(),
                                          value,
                                          type.c_str(),
                                          frequency);
    }

    // A valid metric must be at least 6 chars in length
    if (string_len < 6) {
        //TODO: we dont have access to the error message here
        return;
    }

    // Trim the trailing null chars
    // TODO: perf improvement: send the len along and keep the buffer around as a member variable
    buffer.resize(std::min(string_len, static_cast<int>(buffer.size())));

    // Send the message via the UDP sender
    m_sender->send(buffer);
}

inline void StatsdClient::seed(unsigned int seed) noexcept {
    m_random_engine.seed(seed);
}

}  // namespace Statsd

#endif
