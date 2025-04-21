#ifndef STATSD_CLIENT_HPP
#define STATSD_CLIENT_HPP

#include <cpp-statsd-client/UDPSender.hpp>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace Statsd {

/*!
 *
 * Statsd client
 *
 * This is the Statsd client, exposing the classic methods
 * and relying on a UDP sender for the actual sending.
 *
 * The prefix for a stat is provided once at construction or
 * on reconfiguring the client. The separator character '.'
 * is automatically inserted between the prefix and the stats
 * key, therefore you should neither append one to the prefix
 * nor prepend one to the key
 *
 * The sampling frequency is specified per call and uses a
 * random number generator to determine whether or not the stat
 * will be recorded this time or not.
 *
 * The top level configuration includes 2 optional parameters
 * that determine how the stats are delivered to statsd. These
 * parameters are the batching size and the send interval.
 *
 * The batching size controls the number of bytes to send
 * in each UDP datagram to statsd. This is not a hard limit as
 * we continue appending to a batch of stats until the limit
 * has been reached or surpassed. When this occurs we add the
 * batch to a queue and create a new batch to appended to. A
 * value of 0 for the batching size will disable batching such
 * that each stat will be sent to the daemon individually.
 *
 * The send interval controls the rate at which queued batches
 * of stats will be sent to statsd. If batching is disabled,
 * this value is ignored and each individual stat is sent to
 * statsd immediately in a blocking fashion. If batching is
 * enabled (ie. non-zero) then the send interval is the number
 * of milliseconds to wait before flushing the queue of
 * batched stats messages to the daemon. This is done in a non-
 * blocking fashion via a background thread. If the send
 * interval is 0 then the stats messages are appended to a
 * queue until the caller manually flushes the queue via the
 * flush method.
 *
 */
class StatsdClient {
public:
    //!@name Constructor and destructor, non-copyable
    //!@{

    //! Constructor
    StatsdClient(const std::string& host,
                 const uint16_t port,
                 const std::string& prefix,
                 const uint64_t batchsize = 0,
                 const uint64_t sendInterval = 1000,
                 const int gaugePrecision = 4,
                 unsigned int seed = std::random_device()()) noexcept;

    StatsdClient(const StatsdClient&) = delete;
    StatsdClient& operator=(const StatsdClient&) = delete;

    //!@}

    //!@name Methods
    //!@{

    //! Returns the error message as an std::string
    const std::string& errorMessage() const noexcept;

    //! Increments the key, at a given frequency rate
    void increment(const std::string& key,
                   float frequency = 1.0f,
                   const std::vector<std::string>& tags = {}) const noexcept;

    //! Increments the key, at a given frequency rate
    void decrement(const std::string& key,
                   float frequency = 1.0f,
                   const std::vector<std::string>& tags = {}) const noexcept;

    //! Adjusts the specified key by a given delta, at a given frequency rate
    void count(const std::string& key,
               const int delta,
               float frequency = 1.0f,
               const std::vector<std::string>& tags = {}) const noexcept;

    //! Records a gauge for the key, with a given value, at a given frequency rate
    template <typename T>
    void gauge(const std::string& key,
               const T value,
               float frequency = 1.0f,
               const std::vector<std::string>& tags = {}) const noexcept;

    //! Records a timing for a key, at a given frequency
    void timing(const std::string& key,
                const unsigned int ms,
                float frequency = 1.0f,
                const std::vector<std::string>& tags = {}) const noexcept;

    //! Records a count of unique occurrences for a key, at a given frequency
    void set(const std::string& key,
             const unsigned int sum,
             float frequency = 1.0f,
             const std::vector<std::string>& tags = {}) const noexcept;

    //! Records a custom metric type for the key, with a given value, at a given frequency
    template <typename T>
    void custom(const std::string& key,
                const T value,
                const char* type,
                float frequency = 1.0f,
                const std::vector<std::string>& tags = {}) const noexcept;

    //! Flush any queued stats to the daemon
    void flush() const noexcept;

    //!@}

private:
    // @name Private methods
    // @{

    //! Send a value for a key, according to its type, at a given frequency
    template <typename T>
    void send(const std::string& key,
              const T value,
              const char* type,
              float frequency,
              const std::vector<std::string>& tags) const noexcept;

    //!@}

private:
    //! The prefix to be used for metrics
    std::string m_prefix;

    //! The UDP sender to be used for actual sending
    std::unique_ptr<UDPSender> m_sender;

    //! Fixed floating point precision of gauges
    int m_gaugePrecision;
};

namespace detail {
inline std::string sanitizePrefix(std::string prefix) {
    // For convenience we provide the dot when generating the stat message
    if (!prefix.empty() && prefix.back() == '.') {
        prefix.pop_back();
    }
    return prefix;
}

std::mt19937& rng(unsigned int seed = 0){
    static thread_local std::mt19937 twister(seed);
    return twister;
}

// All supported metric types
constexpr char METRIC_TYPE_COUNT[] = "c";
constexpr char METRIC_TYPE_GAUGE[] = "g";
constexpr char METRIC_TYPE_TIMING[] = "ms";
constexpr char METRIC_TYPE_SET[] = "s";
}  // namespace detail

inline StatsdClient::StatsdClient(const std::string& host,
                                  const uint16_t port,
                                  const std::string& prefix,
                                  const uint64_t batchsize,
                                  const uint64_t sendInterval,
                                  const int gaugePrecision
                                  const unsigned int seed) noexcept
    : m_prefix(detail::sanitizePrefix(prefix)),
      m_sender(new UDPSender{host, port, batchsize, sendInterval}),
      m_gaugePrecision(gaugePrecision) {
    // Initialize the random generator to be used for sampling
    rng(seed);
}

inline void StatsdClient::setConfig(const std::string& host,
                                    const uint16_t port,
                                    const std::string& prefix,
                                    const uint64_t batchsize,
                                    const uint64_t sendInterval,
                                    const int gaugePrecision) noexcept {
    m_prefix = detail::sanitizePrefix(prefix);
    m_sender.reset(new UDPSender(host, port, batchsize, sendInterval));
    m_gaugePrecision = gaugePrecision;
}

inline const std::string& StatsdClient::errorMessage() const noexcept {
    return m_sender->errorMessage();
}

inline void StatsdClient::decrement(const std::string& key,
                                    float frequency,
                                    const std::vector<std::string>& tags) const noexcept {
    count(key, -1, frequency, tags);
}

inline void StatsdClient::increment(const std::string& key,
                                    float frequency,
                                    const std::vector<std::string>& tags) const noexcept {
    count(key, 1, frequency, tags);
}

inline void StatsdClient::count(const std::string& key,
                                const int delta,
                                float frequency,
                                const std::vector<std::string>& tags) const noexcept {
    send(key, delta, detail::METRIC_TYPE_COUNT, frequency, tags);
}

template <typename T>
inline void StatsdClient::gauge(const std::string& key,
                                const T value,
                                const float frequency,
                                const std::vector<std::string>& tags) const noexcept {
    send(key, value, detail::METRIC_TYPE_GAUGE, frequency, tags);
}

inline void StatsdClient::timing(const std::string& key,
                                 const unsigned int ms,
                                 float frequency,
                                 const std::vector<std::string>& tags) const noexcept {
    send(key, ms, detail::METRIC_TYPE_TIMING, frequency, tags);
}

inline void StatsdClient::set(const std::string& key,
                              const unsigned int sum,
                              float frequency,
                              const std::vector<std::string>& tags) const noexcept {
    send(key, sum, detail::METRIC_TYPE_SET, frequency, tags);
}

template <typename T>
inline void StatsdClient::custom(const std::string& key,
                                 const T value,
                                 const char* type,
                                 const float frequency,
                                 const std::vector<std::string>& tags) const noexcept {
    send(key, value, type, frequency, tags);
}

template <typename T>
inline void StatsdClient::send(const std::string& key,
                               const T value,
                               const char* type,
                               float frequency,
                               const std::vector<std::string>& tags) const noexcept {
    // Bail if we can't send anything anyway
    if (!m_sender->initialized()) {
        return;
    }

    // A valid frequency is: 0 <= f <= 1
    // At 0 you never emit the stat, at 1 you always emit the stat and with anything else you roll the dice
    frequency = std::max(std::min(frequency, 1.f), 0.f);
    constexpr float epsilon{0.0001f};
    const bool isFrequencyOne = std::fabs(frequency - 1.0f) < epsilon;
    const bool isFrequencyZero = std::fabs(frequency) < epsilon;
    if (isFrequencyZero ||
        (!isFrequencyOne && (frequency < std::uniform_real_distribution<float>(0.f, 1.f)(rng())))) {
        return;
    }

    // Format the stat message
    std::stringstream valueStream;
    valueStream << std::fixed << std::setprecision(m_gaugePrecision) << value;

    // the thread keeps this buffer around and reuses it, clear should be O(1)
    // and reserve should only have to do so the first time, after that, it's a no-op
    static thread_local std::string buffer;
    buffer.clear();
    buffer.reserve(256);

    buffer.append(m_prefix);
    if (!m_prefix.empty() && !key.empty()) {
        buffer.push_back('.');
    }

    buffer.append(key);
    buffer.push_back(':');
    buffer.append(valueStream.str());
    buffer.push_back('|');
    buffer.append(type);

    if (frequency < 1.f) {
        buffer.append("|@0.");
        buffer.append(std::to_string(static_cast<int>(frequency * 100)));
    }

    if (!tags.empty()) {
        buffer.append("|#");
        for (const auto& tag : tags) {
            buffer.append(tag);
            buffer.push_back(',');
        }
        buffer.pop_back();
    }

    // Send the message via the UDP sender
    m_sender->send(buffer);
}

inline void StatsdClient::flush() const noexcept {
    m_sender->flush();
}

}  // namespace Statsd

#endif
