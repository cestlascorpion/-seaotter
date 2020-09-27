#include "ReportServer.h"
#include "CallReport.pb.h"
#include "RingLog.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace std;

namespace scorpion {

ReportServer::ReportServer()
    : m_flkFd(-1)
    , m_soxFd(-1)
    , m_locked(false)
    , m_running(true)
    , m_buffer(new (nothrow) char[SOX_BUFFER_SIZE])
    , m_shmOp(new (nothrow) ShmClient)
    , m_interval(PERSIST_INTERVAL)
    , m_counter(BUCKET_SIZE) {}

ReportServer::~ReportServer() {
    if (!m_locked) {
        return;
    }
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_soxFd != -1) {
        ::close(m_soxFd);
        m_soxFd = -1;
        unlink(soxPath);
    }
    if (unLockFile()) {
        ::close(m_flkFd);
        m_flkFd = -1;
        unlink(flkPath);
    }
}

ReportServer *ReportServer::Instance() {
    static ReportServer instance;
    return &instance;
}

int ReportServer::Init() {
    int ret = 0;
    static once_flag flag;
    call_once(flag, [this, &ret]() -> void { ret = init(); });
    return ret;
}

int ReportServer::init() {
    if (m_buffer == nullptr) {
        LOG_ERROR("buffer is nullptr.");
        return -1;
    }

    if (m_shmOp == nullptr) {
        LOG_ERROR("shmop is nullptr.");
        return -1;
    }

    if (lockFile() != 0) {
        LOG_ERROR("lockFile() fail.");
        return -1;
    }

    m_soxFd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (m_soxFd == -1) {
        LOG_ERROR("socket() fail, errno %s.", strerror(errno));
        return -1;
    }

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, soxPath, sizeof(addr.sun_path) - 1);

    unlink(soxPath);

    auto length = strlen(addr.sun_path) + sizeof(addr.sun_family);
    if (::bind(m_soxFd, (sockaddr *)&addr, (socklen_t)length) < 0) {
        LOG_ERROR("bind() fail, errno %s.", strerror(errno));
        return -1;
    }

    m_thread = thread(&ReportServer::persistFunc, this);
    return 0;
}

int ReportServer::Run() {
    while (m_running) {
        memset(m_buffer.get(), 0, SOX_BUFFER_SIZE);
        auto recv = ::recv(m_soxFd, m_buffer.get(), SOX_BUFFER_SIZE, 0);
        if (recv < 0) {
            LOG_ERROR("recv() fail, errno %s.", strerror(errno));
            return -1;
        }

        Reports report;
        if (!report.ParseFromArray(m_buffer.get(), (int)recv)) {
            LOG_ERROR("ParseFromArray() fail.");
            return -1;
        }

        for (int idx = 0; idx < report.info_size(); ++idx) {
            const auto &info = report.info(idx);
            const auto key = (uint64_t)info.ip() << 32u | (uint64_t)info.port();
            const usage value{info.total(), info.success()};
            m_counter.AddOrUpdate(key, value, [](usage &b, const usage &e) {
                b.total += e.total;
                b.success += e.success;
            });
        }

        LOG_DEBUG("recv() %ld byte.", recv);
    }
    return 0;
}

void ReportServer::Stop() {
    m_running = false;
}

int ReportServer::lockFile() {
    m_flkFd = open(flkPath, O_RDWR | O_CREAT, 0664);
    if (m_flkFd == -1) {
        LOG_ERROR("open() fail, errno %s.", strerror(errno));
        return -1;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = -1;

    if (fcntl(m_flkFd, F_SETLK, &lock) != 0) {
        LOG_ERROR("fcntl() fail, errno %s.", strerror(errno));
        return -1;
    }
    m_locked = true;
    return 0;
}

int ReportServer::unLockFile() const {
    if (!m_locked) {
        LOG_ERROR("not locked.");
        return -1;
    }

    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = -1;

    if (fcntl(m_flkFd, F_SETLK, &lock) != 0) {
        LOG_ERROR("fcntl() fail, errno %s.", strerror(errno));
        return -1;
    }
    return 0;
}

void ReportServer::persistFunc() {
    while (m_running) {
        auto begin = time(nullptr);
        this_thread::sleep_for(chrono::milliseconds(m_interval));
        auto end = time(nullptr);

        auto data = m_counter.GetAndClear();
        for (const auto &elem : data) {
            m_shmOp->Set((uint32_t)(elem.first >> 32u), (uint16_t)(0Xffffu & elem.first), elem.second.total,
                         elem.second.success);
        }
        m_shmOp->Final((uint64_t)begin, (uint64_t)end);
    }
}

} // namespace scorpion
