#ifndef SEAOTTER_SHMCLIENT_H
#define SEAOTTER_SHMCLIENT_H

#include "RingLog.h"
#include <arpa/inet.h>
#include <unistd.h>

namespace cestlascorpion {

class ShmClient {
public:
    static int Get(uint32_t ip, uint16_t port, std::string &info) {
        LOG_DEBUG("ip %u port %u.", ip, port);
        info = ipToStr(ip) + ":" + std::to_string(port) + "_test_info_";
        return 0;
    }

    static int Set(uint32_t ip, uint16_t port, uint32_t total, uint32_t success) {
        LOG_DEBUG("ip %s port %u total %u success %u.", ipToStr(ip).c_str(), port, total, success);
        return 0;
    }

    static void Final(uint64_t begin, uint64_t end) {
        LOG_DEBUG("last %lu Final.", end - begin);
    }

private:
    static inline std::string ipToStr(uint32_t ip) {
        sockaddr_in addr;
        addr.sin_addr.s_addr = htonl(ip);
        char str[INET_ADDRSTRLEN];
        const char *ptr = inet_ntop(AF_INET, &addr.sin_addr, str, sizeof(str));
        std::string res;
        if (ptr != nullptr) {
            res = ptr;
        } else {
            res = "bad address";
        }
        return res;
    }
};

} // namespace cestlascorpion

#endif // SEAOTTER_SHMCLIENT_H
