#include <StatsdClient.hpp>
#include <StatsdServer.hpp>
#include <iostream>

using namespace Statsd;

// We just keep storing metrics in an vector until we hear a special one, then we bail
void mock(StatsdServer& server, std::vector<std::string>& messages) {
    do {
        // Grab the messages that are waiting
        auto recvd = server.receive();

        // Split the messages on '\n'
        std::string::size_type start = -1;
        do {
            // Keep this message
            auto end = recvd.find('\n', ++start);
            messages.emplace_back(recvd.substr(start, end));
            start = end;

            // Bail if we found the special quit message
            if (messages.back().find("DONE") != std::string::npos) {
                messages.pop_back();
                return;
            }
        } while (start != std::string::npos);
    } while (server.errorMessage().empty() && !messages.back().empty());
}

int main(int argc, char** argv) {
    // Spawn a background thread to listen for the messages over UDP as if it were a real statsd server
    // Note that we could just synchronously recv metrics and not use a thread but doing the test async
    // has the advantage that we can add tests for the batching mode and we dont have to worry about
    // how to test that synchronously
    StatsdServer mock_server;
    std::vector<std::string> messages;
    std::thread server(mock, std::ref(mock_server), std::ref(messages));

    // Connect to a rubbish ip and make sure initialization failed
    StatsdClient client{"256.256.256.256", 8125, "myPrefix.", 20};
    if (client.errorMessage().empty()) {
        std::cerr << "should not be able to connect to ridiculous ip" << std::endl;
        return EXIT_FAILURE;
    }

    // Set a new config that actually connects to the running server
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
    client.seed(19); // this seed gets a hit on the first call
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
    client.seed(19);
    client.timing("myTiming", 2, 0.1f);
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    // TODO: should sampling rates above 1 error or be capped?
    // Send a metric explicitly
    client.send("tutu", 4, "c", 2.0f);
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    // TODO: reconfigure and test batching deamon

    // Signal the mock server we are done
    client.send("DONE", 0, "DONE");

    // Wait for the server to stop
    server.join();

    // Make sure we get the exactly correct output
    std::vector<std::string> expected = {
        "myPrefix.coco:1|c",
        "myPrefix.kiki:-1|c",
        "myPrefix.toto:2|c|@0.10",
        "myPrefix.titi:3|g",
        "myPrefix.myTiming:2|ms|@0.10",
        "myPrefix.tutu:4|c|@2.00",
    };
    if(messages != expected) {
        std::cerr << "Unexpected messages received by server, got:" << std::endl;
        for(const auto& message : messages) {
            std::cerr << message << std::endl;
        }
        std::cerr << std::endl << "But we expected:" << std::endl;
        for(const auto& message : expected) {
            std::cerr << message << std::endl;
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
