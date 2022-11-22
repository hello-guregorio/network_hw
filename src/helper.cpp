#include "helper.h"

#include <sys/stat.h>

#include <chrono>
#include <iostream>
uint16_t checksum(uint16_t* buf, int nwords) {
    uint32_t sum;

    for (sum = 0; nwords > 0; nwords--) sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}
size_t file_size(const char* file_name) {
    if (file_name == nullptr) return 0;
    struct stat statbuf;
    stat(file_name, &statbuf);
    return statbuf.st_size;
}
bool tiny_timer::time_out() {
    std::chrono::time_point<std::chrono::high_resolution_clock> now =
        std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - start) >=
        std::chrono::seconds(timeout);
}

void tiny_timer::restart(int time_out) {
    timeout = time_out;
    start = std::chrono::high_resolution_clock::now();
}


int quick_pow(int a, int exp) {
    int ans = 1;
    while (exp) {
        if (1 & exp) {
            ans *= a;
        }
        exp >>= 1;
        a *= a;
    }
    return ans;
}