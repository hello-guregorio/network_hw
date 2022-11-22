#pragma once
#define MSS 1024  // 数据段最大长度1024字节
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>

enum flag_field { SYN, SYN_ACK, ACK, DATA, FIN, FIN_ACK, TRANS_OVER, RST };

// 报头
struct header {
  uint16_t seq_num;
  uint16_t ack_num;
  uint16_t cksum;
  flag_field flags;
  uint32_t datalen;
  header() { memset(this, 0, sizeof(header)); }
  void set_hdr(uint16_t seqnum, uint16_t acknum, flag_field flag,
               uint32_t data_size) {
    seq_num = seqnum;
    ack_num = acknum;
    flags = flag;
    datalen = data_size;
    cksum = 0;
    cksum = checksum((uint16_t*)this, sizeof(header) / sizeof(uint16_t));
  }
};

// 报文段
struct packet {
  header hdr;
  char data[MSS];
  packet() { memset(this->data, 0, MSS); }
};