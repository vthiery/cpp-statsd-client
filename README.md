# C++ StatsD Client

[ ![Download](https://api.bintray.com/packages/vthiery/conan-packages/statsdclient%3Avthiery/images/download.svg) ](https://bintray.com/vthiery/conan-packages/statsdclient%3Avthiery/_latestVersion)
[![Build Status](https://travis-ci.org/vthiery/cpp-statsd-client.svg?branch=develop)](https://travis-ci.org/vthiery/cpp-statsd-client)
[![Github Issues](https://img.shields.io/github/issues/vthiery/cpp-statsd-client.svg)](https://github.com/vthiery/cpp-statsd-client/issues)
[![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/vthiery/cpp-statsd-client.svg)](http://isitmaintained.com/project/vthiery/cpp-statsd-client "Average time to resolve an issue")

A header-only StatsD client implemented in C++.
The client allows:
- batching,
- change of configuration at runtime,
- user-defined frequency rate.

## Install and Test

### Makefile

In order to install the header files and/or run the tests, simply use the Makefile and execute

```
 make install
 ```

and

```
 make test
 ```

### Conan

If you are using [Conan](https://www.conan.io/) to manage your dependencies, merely add statsdclient/x.y.z@vthiery/stable to your conanfile.py's requires, where x.y.z is the release version you want to use. Please file issues here if you experience problems with the packages. You can also directly download the latest version [here](https://bintray.com/vthiery/conan-packages/statsdclient%3Avthiery/_latestVersion).

## Usage
### Example
A simple example of how to use the client:

```cpp
#include "StatsdClient.hpp"
using namespace Statsd;

int main()
{
    // Define the client on localhost, with port 8080, using a prefix and a batching size of 20 bytes
    StatsdClient client{ "127.0.0.1", 8080, "myPrefix.", 20 };

    // Increment the metric "coco"
    client.increment("coco");

    // Decrement the metric "kiki"
    client.decrement("kiki");

    // Adjusts "toto" by +3
    client.count("toto", 2, 0.1f);

    // Record a gauge "titi" to 3
    client.gauge("titi", 3);

    // Record a timing of 2ms for "myTiming"
    client.timing("myTiming", 2, 0.1f);

    // Send a metric explicitly
    client.send("tutu", 4, "c", 2.0f);
}
```

### Configuration

The configuration of the client must be input when one instantiates it. Nevertheless, the API allows the configuration ot change afterwards. For example, one can do the following:

```cpp
#include "StatsdClient.hpp"
using namespace Statsd;

int main()
{
    // Define the client on localhost, with port 8080, using a prefix and a batching size of 20 bytes
    StatsdClient client{ "127.0.0.1", 8080, "myPrefix.", 20 };

    client.increment("coco");

    // Set a new configuration, using a different port and a different prefix
    client.setConfig("127.0.0.1", 8000, "anotherPrefix.");

    client.decrement("kiki");
}
```

The batchsize is the only parameter that cannot be changed for the time being.

### Batching

The client supports batching of the metrics:
* the unit of the batching parameter is bytes,
* it is optional and not setting it will result in an immediate send of any metrics,

If set, the UDP sender will spawn a thread sending the metrics to the server every 1 second by aggregates. The aggregation logic is as follows:
* if the last message has already reached the batching limit, add a new element in a message queue;
* otherwise, append the message to the last one, separated by a `\n`.

### Frequency rate

When sending a metric, a frequency rate can be set in order to limit the metrics' sampling. By default, the frequency rate is set to one and won't affect the sampling. If set to a value `epsilon` (0.0001 for the time being) close to one, the sampling is not affected either.

If the frequency rate is set and `epsilon` different from one, the sending will be rejected randomly (the higher the frequency rate, the lower the probability of rejection).


## License
This library is under MIT license.
