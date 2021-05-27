#include <StatsdClient.hpp>
#include <cassert>

using namespace Statsd;

int main() {
    StatsdClient client{"localhost", 8080, "myPrefix.", 20};

    // set a new config with a different port
    client.setConfig("localhost", 8000, "myPrefix.");

    // Increment "coco"
    client.increment("coco");
    assert(client.errorMessage().empty());

    // Decrement "kiki"
    client.decrement("kiki");
    assert(client.errorMessage().empty());

    // Adjusts "toto" by +3
    client.count("toto", 2, 0.1f);
    assert(client.errorMessage().empty());

    // Record a gauge "titi" to 3
    client.gauge("titi", 3);
    assert(client.errorMessage().empty());

    // Record a timing of 2ms for "myTiming"
    client.timing("myTiming", 2, 0.1f);
    assert(client.errorMessage().empty());

    // Send a metric explicitly
    client.send("tutu", 4, "c", 2.0f);
    assert(client.errorMessage().empty());

    return EXIT_SUCCESS;
}
