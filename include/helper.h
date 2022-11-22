#pragma once
#include <chrono>
#include <cstdint>
/*У���*/
uint16_t checksum(uint16_t* buf, int nwords);
/*��ȡ�ļ���С,��λ�ֽ�*/
size_t file_size(const char* file_name);
/*������*/
int quick_pow(int a, int exp);
/*��ʱ��*/
class tiny_timer {
 public:
  // ��ʱ����true
  bool time_out();
  void restart(int time_out);

 private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  int timeout;
};