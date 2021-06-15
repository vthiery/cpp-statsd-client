#include <iostream>

#include "StatsdServer.hpp"
#include "cpp-statsd-client/StatsdClient.hpp"

using namespace Statsd;

// Each test suite below spawns a thread to recv the client messages over UDP as if it were a real statsd server
// Note that we could just synchronously recv metrics and not use a thread but doing the test async has the
// advantage that we can test the threaded batching mode in a straightforward way. The server thread basically
// just keeps storing metrics in an vector until it hears a special one signaling the test is over and bails
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

// TODO: would it be ok if we change the API of the client to throw on error instead of having to check a string?
void throwOnError(const StatsdClient& client) {
    if (!client.errorMessage().empty()) {
        std::cerr << client.errorMessage() << std::endl;
        throw std::runtime_error(client.errorMessage());
    }
}

void testErrorConditions() {
    // Connect to a rubbish ip and make sure initialization failed
    StatsdClient client{"256.256.256.256", 8125, "myPrefix.", 20};
    if (client.errorMessage().empty()) {
        std::cerr << "Should not be able to connect to ridiculous ip" << std::endl;
        throw std::runtime_error("Should not be able to connect to ridiculous ip");
    }
}

void testReconfigure() {
    StatsdServer server;

    StatsdClient client("localhost", 8125, "first.");
    client.send("foo", 1, "c", 1.f);
    if (server.receive() != "first.foo:1|c") {
        throw std::runtime_error("Incorrect stat received");
    }

    client.setConfig("localhost", 8125, "second");
    client.send("bar", 1, "c", 1.f);
    if (server.receive() != "second.bar:1|c") {
        throw std::runtime_error("Incorrect stat received");
    }

    client.setConfig("localhost", 8125, "");
    client.send("third.baz", 1, "c", 1.f);
    if (server.receive() != "third.baz:1|c") {
        throw std::runtime_error("Incorrect stat received");
    }

    client.send("", 1, "c", 1.f);
    if (server.receive() != ":1|c") {
        throw std::runtime_error("Incorrect stat received");
    }

    // TODO: test what happens with the batching after resolving the question about incomplete
    //  batches being dropped vs sent on reconfiguring
}

void testSendRecv(uint64_t batchSize) {
    StatsdServer mock_server;
    std::vector<std::string> messages, expected;
    std::thread server(mock, std::ref(mock_server), std::ref(messages));

    // Set a new config that has the client send messages to a proper address that can be resolved
    StatsdClient client("localhost", 8125, "sendRecv.", batchSize);
    throwOnError(client);

    // TODO: I forget if we need to wait for the server to be ready here before sending the first stats
    //  is there a race condition where the client sending before the server binds would drop that clients message

    for (int i = 0; i < 3; ++i) {
        // Increment "coco"
        client.increment("coco");
        throwOnError(client);
        expected.emplace_back("sendRecv.coco:1|c");

        // Decrement "kiki"
        client.decrement("kiki");
        throwOnError(client);
        expected.emplace_back("sendRecv.kiki:-1|c");

        // Adjusts "toto" by +2
        client.seed(19);  // this seed gets a hit on the first call
        client.count("toto", 2, 0.1f);
        throwOnError(client);
        expected.emplace_back("sendRecv.toto:2|c|@0.10");

        // Gets "sampled out" by the random number generator
        client.count("popo", 9, 0.1f);
        throwOnError(client);

        // Record a gauge "titi" to 3
        client.gauge("titi", 3);
        throwOnError(client);
        expected.emplace_back("sendRecv.titi:3|g");

        // Record a timing of 2ms for "myTiming"
        client.seed(19);
        client.timing("myTiming", 2, 0.1f);
        throwOnError(client);
        expected.emplace_back("sendRecv.myTiming:2|ms|@0.10");

        // Send a metric explicitly
        client.send("tutu", 241, "c", 2.0f);
        throwOnError(client);
        expected.emplace_back("sendRecv.tutu:241|c");
    }

    // Signal the mock server we are done
    client.send("DONE", 0, "ms");

    // Wait for the server to stop
    server.join();

    // Make sure we get the exactly correct output
    if (messages != expected) {
        std::cerr << "Unexpected stats received by server, got:" << std::endl;
        for (const auto& message : messages) {
            std::cerr << message << std::endl;
        }
        std::cerr << std::endl << "But we expected:" << std::endl;
        for (const auto& message : expected) {
            std::cerr << message << std::endl;
        }
        throw std::runtime_error("Unexpected stats");
    }
}

int main() {
    // If any of these tests fail they throw an exception, not catching makes for a nonzero return code

    // general things that should be errors
    testErrorConditions();
    // reconfiguring how you are sending
    testReconfigure();
    // no batching
    testSendRecv(0);
    // background batching
    testSendRecv(4);

    return EXIT_SUCCESS;
}
