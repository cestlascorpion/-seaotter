#include "ReportServer.h"
#include "RingLog.h"

#include <cassert>
#include <chrono>
#include <csignal>

using namespace std;
using namespace scorpion;

void SignalStop(int) {
    LOG_ERROR("[Signal] stop.");
    ReportServer::Instance()->Stop();
}

void SignalHandler() {
    assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR);
    assert(signal(SIGALRM, SIG_IGN) != SIG_ERR);
    assert(signal(SIGCHLD, SIG_IGN) != SIG_ERR);
    assert(sigset(SIGINT, SignalStop) != SIG_ERR);
    assert(sigset(SIGTERM, SignalStop) != SIG_ERR);
    assert(sigset(SIGHUP, SignalStop) != SIG_ERR);
}

int main() {
    LOG_INIT(".", "server", DEBUG);
    SignalHandler();
    if (ReportServer::Instance()->Init() != 0) {
        LOG_ERROR("[Init] fail.");
        return 0;
    }
    ReportServer::Instance()->Run();
    this_thread::sleep_for(chrono::seconds(1));
    return 0;
}
