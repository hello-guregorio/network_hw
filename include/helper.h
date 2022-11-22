#pragma once
#include <chrono>
#include <cstdint>
/*校验和*/
uint16_t checksum(uint16_t* buf, int nwords);
/*获取文件大小,单位字节*/
size_t file_size(const char* file_name);
/*快速幂*/
int quick_pow(int a, int exp);
/*计时器*/
class tiny_timer {
 public:
  // 超时返回true
  bool time_out();
  void restart(int time_out);

 private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  int timeout;
};