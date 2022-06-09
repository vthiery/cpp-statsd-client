# C++ StatsD Client

![logo](https://raw.githubusercontent.com/vthiery/cpp-statsd-client/master/images/logo.svg?sanitize=true)

[![Release](https://img.shields.io/github/release/vthiery/cpp-statsd-client.svg?style=for-the-badge)](https://github.com/vthiery/cpp-statsd-client/releases/latest)
![License](https://img.shields.io/github/license/vthiery/cpp-statsd-client?style=for-the-badge)
[![Linux status](https://img.shields.io/github/workflow/status/vthiery/cpp-statsd-client/Linux?label=Linux&style=for-the-badge)](https://github.com/vthiery/cpp-statsd-client/actions/workflows/linux.yml?query=branch%3Amaster++)
[![Windows status](https://img.shields.io/github/workflow/status/vthiery/cpp-statsd-client/Windows?label=Windows&style=for-the-badge)](https://github.com/vthiery/cpp-statsd-client/actions/workflows/windows.yml?query=branch%3Amaster++)

A header-only StatsD client implemented in C++.
The client allows:

- batching,
- change of configuration at runtime,
- user-defined frequency rate.

## Install and Test

In order to install the header files and/or run the tests, simply use the Makefile and execute

```sh
make test
```

## Usage

### Simple example

A simple example of how to use the client:

```cpp
#include "StatsdClient.hpp"
using namespace Statsd;

int main() {
    // Define the client on localhost, with port 8080 and using a prefix
    StatsdClient client{"localhost", 8080, "myPrefix"};

    // Increment the metric "coco"
    client.increment("coco");

    // Decrement the metric "kiki"
    client.decrement("kiki");

    // Adjusts "toto" by +3
    client.count("toto", 2, 0.1f);

    // Record a gauge "titi" to 3.2 with tags
    client.gauge("titi", 3.2, 0.1f, {"foo", "bar"});

    // Record a timing of 2ms for "myTiming"
    client.timing("myTiming", 2, 0.1f);

    // Record a count of unique occurences of "tutu"
    client.set("tutu", 4, 2.0f);

    exit(0);
}
```

### Client configuration

The configuration of the client must be input when one instantiates it. Nevertheless, the API allows the configuration ot change afterwards. For example, one can do the following:

```cpp
#include "StatsdClient.hpp"
using namespace Statsd;

int main()
{
    // Define the client on localhost, with port 8080 and using a prefix
    StatsdClient client{"localhost", 8080, "myPrefix"};

    client.increment("coco");

    // Set a new configuration, using a different port and a different prefix
    client.setConfig("localhost", 8081, "anotherPrefix");

    client.decrement("kiki");
}
```

#### Batching

The client supports batching of the metrics. The batch size parameter is the number of bytes to allow in each batch (UDP datagram payload) to be sent to the statsd process. E.g.

```cpp
StatsdClient client{"localhost", 8080, "myPrefix", 64};
```

This number is not a hard limit. If appending the current stat to the current batch (separated by the `'\n'` character) pushes the current batch over the batch size, that batch is enqueued (not sent) and a new batch is started. If batch size is 0, the default, then each stat is sent individually to the statsd process and no batches are enqueued.

##### Sending interval

As previously mentioned, if batching is disabled (by setting the batch size to 0) then every stat is sent immediately in a blocking fashion.

If batching is enabled (ie non-zero) then you may also set the send interval. E.g.

```cpp
StatsdClient client{"localhost", 8080, "myPrefix", 64, 1000};
```

The send interval controls the time, in milliseconds, to wait before flushing/sending the queued stats batches to the statsd process. When the send interval is non-zero a background thread is spawned which will do the flushing/sending at the configured send interval, in other words asynchronously. The queuing mechanism in this case is _not_ lock-free. If batching is enabled but the send interval is set to zero then the queued batchs of stats will not be sent automatically by a background thread but must be sent manually via the `flush` method. The `flush` method is a blocking call.

#### Gauge precision

Since gauge metrics support floats, it may be useful to configure the desired precision. The client support this via the `gaugePrecision` parameter. E.g.

```cpp
StatsdClient client{"localhost", 8080, "myPrefix", 0, 0, 4};
```

which would set a precision of `4` on every gauge value.

### Options when sending metrics

#### Frequency rate

When sending a metric, a frequency rate can be set in order to limit the metrics' sampling. E.g.

```cpp
client.gauge("titi", 3.2, 0.1f);
```

By default, the frequency rate is set to one and won't affect the sampling. If set to a value `epsilon` (0.0001 for the time being) close to one, the sampling is not affected either.

If the frequency rate is set and `epsilon` different from one, the sending will be rejected randomly (the higher the frequency rate, the lower the probability of rejection).

#### Tags

One may also attach tags to a metrics, e.g.

```cpp
client.gauge("titi", 3.2, 0.1f, {"foo", "bar"});
```

### Custom metric types

Some statsd backends (e.g. Datadog, netdata) support metric types beyond those supported by the original Etsy statsd daemon, e.g.

* Histograms (type "h"), which are like Timers, but with different units
* Dictionaries (type "d"), which are like Sets but also record a count for each unique value

Non-standard metrics can be sent using the `custom()` method, where the metric type is passed in by the caller.  The client performs no validation, under the assumption that the caller will only pass in types, values, and other optional parameters (e.g. frequency, tags) that are supported by their backend.  The templated `value` parameter must support serialization to a `std::basic_ostream` using `operator<<`.

```cpp
// Record a histogram of packet sizes
client.custom("packet_size", 1500, "h");
```

## Advanced Testing

A simple mock StatsD server can be found at `tests/StatsdServer.hpp`. This can be used to do simple validation of your application's metrics, typically in the form of unit tests. In fact this is the primary means by which this library is tested. The mock server itself is not distributed with the library so to use it you'd need to vendor this project into your project. Once you have though, you can test your application's use of the client like so:

```cpp
#include "StatsdClient.hpp"
#include "StatsdServer.hpp"

#include <cassert>

using namespace Statsd;

struct MyApp {
    void doWork() const {
        m_client.count("bar", 3);
    }
private:
    StatsdClient m_client{"localhost", 8125, "foo"};
};

int main() {
    StatsdServer mockServer;

    MyApp app;
    app.doWork();

    assert(mockServer.receive() == "foo.bar:3|c");
    exit(0);
}
```

## License

This library is under MIT license.
