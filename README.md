# C++ StatsD Client

A header-only StatsD client implemented in C++.
The client allows:
- batching,
- change of configuration at runtime,
- user-define frequency rate.

## Usage
A simple example of how to use the client:

```cpp
#include "StatsdClient.hpp"
using namespace Statsd;

int main()
{
    // Define the client
    StatsdClient client{ "127.0.0.1", 5005, "myPrefix_", true };

    // Increment "coco"
    client.increment("coco");

    // Decrement "kiki"
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

## License
This library is under MIT license.
