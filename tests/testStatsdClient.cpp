#include <StatsdClient.hpp>
#include <cassert>
#include <iostream>

using namespace Statsd;

int main(int argc, char** argv) {
    // connect to a rubbish port and make sure initialization failed
    StatsdClient client{"256.256.256.256", 8125, "myPrefix.", 20};
    if (client.errorMessage().empty()) {
        std::cerr << "should not be able to connect to ridiculous ip" << std::endl;
        return EXIT_FAILURE;
    }

    // set a new config with a different port
    client.setConfig("localhost", 8125, "myPrefix.");
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    // Increment "coco"
    client.increment("coco");
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    // Decrement "kiki"
    client.decrement("kiki");
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    // Adjusts "toto" by +3
    client.count("toto", 2, 0.1f);
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    // Record a gauge "titi" to 3
    client.gauge("titi", 3);
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    // Record a timing of 2ms for "myTiming"
    client.timing("myTiming", 2, 0.1f);
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    // Send a metric explicitly
    client.send("tutu", 4, "c", 2.0f);
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
