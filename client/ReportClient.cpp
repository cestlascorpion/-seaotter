#include "ReportClient.h"
#include "CallReport.pb.h"
#include "RingLog.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace std;

namespace cestlascorpion {

ReportClient::ReportClient()
    : m_soxFd(-1)
    , m_running(true)
    , m_shmOp(new (nothrow) ShmClient)
    , m_interval(REPORT_INTERVAL)
    , m_counter(BUCKET_SIZE) {}

ReportClient::~ReportClient() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_soxFd != -1) {
        ::close(m_soxFd);
        m_soxFd = -1;
    }
    if (!m_path.empty()) {
        unlink(m_path.c_str());
    }
}

ReportClient *ReportClient::Instance() {
    static ReportClient instance;
    return &instance;
}

int ReportClient::Init() {
    int ret = 0;
    static once_flag flag;
    call_once(flag, [this, &ret]() -> void { ret = init(); });
    return ret;
}

int ReportClient::init() {
    m_soxFd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (m_soxFd == -1) {
        LOG_ERROR("socket() fail, errno %s.", strerror(errno));
        return -1;
    }

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path) - 1, "%s.%05d", soxPath, getpid());

    m_path = addr.sun_path;
    unlink(addr.sun_path);

    auto length = strlen(addr.sun_path) + sizeof(addr.sun_family);
    if (::bind(m_soxFd, (sockaddr *)&addr, (socklen_t)length) < 0) {
        LOG_ERROR("bind() fail, errno %s.", strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, soxPath);

    length = strlen(addr.sun_path) + sizeof(addr.sun_family);
    if (::connect(m_soxFd, (sockaddr *)&addr, (socklen_t)length) < 0) {
        LOG_ERROR("connect() fail, errno %s.", strerror(errno));
        return -1;
    }

    m_thread = thread(&ReportClient::consumeFunc, this);
    return 0;
}

int ReportClient::Commit(const char *ip, uint16_t port, bool success) {
    return Commit(ipToInt(ip), port, success);
}

int ReportClient::Fetch(const char *ip, uint16_t port, string &info) {
    return Fetch(ipToInt(ip), port, info);
}

int ReportClient::Commit(uint32_t ip, uint16_t port, bool success) {
    const uint64_t key = (uint64_t)ip << 32u | (uint64_t)port;
    const usage value{1, success ? 1u : 0};
    m_counter.AddOrUpdate(key, value, [](usage &b, const usage &e) {
        b.total += e.total;
        b.success += e.success;
    });
    return 0;
}

int ReportClient::Fetch(uint32_t ip, uint16_t port, string &info) {
    if (m_shmOp == nullptr) {
        LOG_ERROR("shmop is nullptr.");
        return -1;
    }

    return m_shmOp->Get(ip, port, info);
}

void ReportClient::consumeFunc() {
    while (true) {
        if (!m_running) {
            LOG_DEBUG("marked as stop.");
            return;
        }

        this_thread::sleep_for(chrono::milliseconds(m_interval));

        do {
            Reports report;
            auto data = m_counter.GetAndClear();
            for (const auto &elem : data) {
                auto *info = report.add_info();
                info->set_ip((uint32_t)(elem.first >> 32u));
                info->set_port((uint16_t)(0xffffu & elem.first));
                info->set_total(elem.second.total);
                info->set_success(elem.second.success);
                if (report.ByteSize() > SEGMENT_SIZE) {
                    if (sendMsg(m_soxFd, report) != 0) {
                        LOG_ERROR("sendMsg() fail.");
                        return;
                    }
                    report.clear_info();
                }
            }

            if (report.info_size() > 0) {
                if (sendMsg(m_soxFd, report) != 0) {
                    LOG_ERROR("sendMsg() fail.");
                    return;
                }
            }
        } while (false);
    }
}

int ReportClient::sendMsg(int soxFd, const Reports &report) {
    LOG_DEBUG("sendMsg() %d byte.", report.ByteSize());

    string message;
    if (!report.SerializeToString(&message)) {
        LOG_ERROR("SerializeToString() fail.");
        return -1;
    }

    if (::send(soxFd, message.c_str(), message.size(), 0) != (ssize_t)message.size()) {
        LOG_ERROR("send() fail, errno %s.", strerror(errno));

        return -1;
    }
    return 0;
}

uint32_t ReportClient::ipToInt(const string &ip) {
    sockaddr_in addr;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
    return ntohl(addr.sin_addr.s_addr);
}

} // namespace cestlascorpion
