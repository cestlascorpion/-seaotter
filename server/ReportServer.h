#ifndef SEAOTTER_REPORTSERVER_H
#define SEAOTTER_REPORTSERVER_H

#include "CanaryCounter.h"
#include "ShmClient.h"
#include <mutex>
#include <thread>

namespace cestlascorpion {

class ReportServer {
public:
    static ReportServer *Instance();
    ~ReportServer();

    ReportServer(const ReportServer &) = delete;
    ReportServer &operator=(const ReportServer) = delete;

public:
    // 初始化 线程安全，不过没有必要
    int Init();
    // 运行
    int Run();
    // 停止，由 signal 处理函数调用
    void Stop();

private:
    ReportServer();

    int init();
    inline int lockFile();
    inline int unLockFile() const;
    void persistFunc();

private:
    struct usage {
        uint32_t total;
        uint32_t success;
    };

private:
    // 创建 flock 的文件地址
    const char *flkPath = "/tmp/sea.otter.flk";
    // 创建监听 unix socket 地址
    const char *soxPath = "/tmp/sea.otter.sox";
    // map 的初始 bucket 数量 | 后台线程聚合上报间隔 | 接收 UDP 报文的缓冲区大小
    enum { BUCKET_SIZE = 557, PERSIST_INTERVAL = 5000, SOX_BUFFER_SIZE = 4096 };
    // key: [ 32bit ip | 32bit port] value: [usage total | usage success]
    using dataMap = CanaryCounter<uint64_t, usage, std::hash<uint64_t>>;

    int m_flkFd;
    int m_soxFd;
    bool m_locked;
    bool m_running;
    std::thread m_thread;
    std::unique_ptr<char[]> m_buffer;
    std::unique_ptr<ShmClient> m_shmOp;
    uint32_t m_interval;
    dataMap m_counter;
};
} // namespace cestlascorpion

#endif // SEAOTTER_REPORTSERVER_H
