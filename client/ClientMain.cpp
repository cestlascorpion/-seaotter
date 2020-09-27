#include "ReportClient.h"
#include "RingLog.h"

#include <netinet/in.h>
#include <random>
#include <vector>

using namespace std;
using namespace scorpion;

inline double Rand(double mean, double stddev) {
    static default_random_engine engine(time(nullptr));
    static normal_distribution<double> normal(mean, stddev);
    return abs(normal(engine));
}

void TestFunc(const vector<string> &ips, const vector<uint16_t> &ports, uint32_t turns) {
    int64_t cost = 0;
    timespec t1, t2;
    for (uint32_t turn = 0; turn < turns; ++turn) {
        auto ip = ips[turn % ips.size()];
        auto port = ports[turn % ports.size()];
        clock_gettime(CLOCK_MONOTONIC, &t1);
        if (ReportClient::Instance()->Commit(ip.c_str(), port, true) != 0) {
            LOG_DEBUG("[Commit] fail.");
            return;
        }
        clock_gettime(CLOCK_MONOTONIC, &t2);
        cost += (t2.tv_nsec - t1.tv_nsec) + (t2.tv_sec - t1.tv_sec) * 1000 * 1000 * 1000;   // ns
        this_thread::sleep_for(chrono::microseconds((uint32_t)(Rand(1.25, 0.5) * 1000.0))); // us
    }
    LOG_DEBUG("[TestFunc] call %d avg %ld ns.", turns, cost / turns);
}

int main(int argc, char *argv[]) {
    LOG_INIT(".", "client", DEBUG);

    const vector<string> ips = {
        "192.168.1.101", "192.168.1.102", "192.168.1.103", "192.168.1.104", "192.168.1.105", "192.168.1.106",
        "192.168.1.107", "192.168.1.108", "192.168.1.109", "192.168.1.110", "192.168.1.111", "192.168.1.112",

        "192.168.2.101", "192.168.2.102", "192.168.2.103", "192.168.2.104", "192.168.2.105", "192.168.2.106",
        "192.168.2.107", "192.168.2.108", "192.168.2.109", "192.168.2.110", "192.168.2.111", "192.168.2.112",

        "192.168.3.101", "192.168.3.102", "192.168.3.103", "192.168.3.104", "192.168.3.105", "192.168.3.106",
        "192.168.3.107", "192.168.3.108", "192.168.3.109", "192.168.3.110", "192.168.3.111", "192.168.3.112",

        "192.168.4.101", "192.168.4.102", "192.168.4.103", "192.168.4.104", "192.168.4.105", "192.168.4.106",
        "192.168.4.107", "192.168.4.108", "192.168.4.109", "192.168.4.110", "192.168.4.111", "192.168.4.112",

    };
    const vector<uint16_t> ports = {1234, 2345, 3456, 4567};

    if (argc != 3) {
        LOG_DEBUG("[Usage] (turns) (thread)\n");
        return 0;
    }
    std::string type = argv[1];
    auto turns = (uint32_t)stoi(argv[1]);
    auto numbers = (uint32_t)stoi(argv[2]);

    if (ReportClient::Instance()->Init() != 0) {
        LOG_DEBUG("[Init] fail.");
        return 0;
    }

    timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    vector<thread> threads(numbers);
    for (auto &t : threads) {
        t = thread(TestFunc, ref(ips), ref(ports), turns);
    }

    for (auto &t : threads) {
        t.join();
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);
    auto cost = (t2.tv_sec - t1.tv_sec) * 1000 + (t2.tv_nsec - t1.tv_nsec) / 1000000; // ms
    LOG_DEBUG("[commit total] use %ld ms qps %ld.", cost, 1000 * turns * numbers / cost);
    this_thread::sleep_for(chrono::seconds(1));
    return 0;
}
