#ifndef SEAOTTER_REPORTCLIENT_H
#define SEAOTTER_REPORTCLIENT_H

#include "CanaryCounter.h"
#include "ShmClient.h"
#include <mutex>
#include <thread>

namespace cestlascorpion {

class Reports;
class ReportClient {
public:
    static ReportClient *Instance();
    ~ReportClient();

    ReportClient(const ReportClient &) = delete;
    ReportClient &operator=(const ReportClient) = delete;

public:
    // 初始化 线程安全
    int Init();
    // 上报调用信息
    int Commit(const char *ip, uint16_t port, bool success);
    // 查询调用情况
    int Fetch(const char *ip, uint16_t port, std::string &info);
    // 对整型 IP 地址的重载版本
    int Commit(uint32_t ip, uint16_t port, bool success);
    // 对整型 IP 地址的重载版本
    int Fetch(uint32_t ip, uint16_t port, std::string &info);

private:
    ReportClient();

    int init();
    void consumeFunc();
    static inline int sendMsg(int soxFd, const Reports &report);
    static inline uint32_t ipToInt(const std::string &ip);

private:
    struct usage {
        uint32_t total;
        uint32_t success;
    };

private:
    // 创建 unix socket 地址前缀，完整格式为 "prefix+pid"，服务端地址只包含前缀
    const char *soxPath = "/tmp/sea.otter.sox";
    // map 的初始 bucket 数量 | 后台线程聚合上报间隔 | >SEGMENT_SIZE 后封装为UDP包发送
    enum { BUCKET_SIZE = 331, REPORT_INTERVAL = 200, SEGMENT_SIZE = 3072 }; // server -> 4096
    // key: [ 32bit ip | 32bit port] value: [usage total | usage success]
    using dataMap = CanaryCounter<uint64_t, usage, std::hash<uint64_t>>;

    int m_soxFd;
    bool m_running;
    std::string m_path;
    std::thread m_thread;
    std::unique_ptr<ShmClient> m_shmOp;
    uint32_t m_interval;
    dataMap m_counter;
};
}; // namespace cestlascorpion

#endif // SEAOTTER_REPORTCLIENT_H
