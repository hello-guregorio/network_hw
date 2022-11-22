#pragma once
#include <WinSock2.h>

#include <iostream>

#include "helper.h"

#pragma comment(lib, "ws2_32.lib")

#define MAX_ATTEMPTS 3  // 最大尝试次数
// tcp握手挥手时的各种状态,用状态机实现
enum state {
  /*客户端用到的状态*/
  CLOSED,       // 关闭          <==创建套接字时初态是这个
  SYN_SENT,     // 发送syn
  ESTABLISHED,  // 连接建立
  FIN_WAIT_1,   // 挥手等待1
  FIN_WAIT_2,   // 挥手等待2
  TIME_WAIT,    // 挥手完后等待时间
  /*服务端用到的状态*/
  LISTEN,      // 握手时监听
  SYN_RCVD,    // 第三次握手时
  CLOSE_WAIT,  // 挥手关闭
  LAST_ACK     // 等最后一个ack
};
// 重载运算符,方便输出
std::ostream& operator<<(std::ostream& out, state stat);

class rdt_socket {
 public:
  rdt_socket();
  /*连接*/
  int connect(SOCKET& sock, SOCKADDR_IN& addr, int& addrlen);
  /*接受*/
  int accept(SOCKET& sock, SOCKADDR_IN& addr, int& addrlen);
  /*send*/
  int send(SOCKET& sock, char* buf, size_t len, SOCKADDR_IN& addr,
           int& addrlen);
  /*recv*/
  int recv(SOCKET& sock, char* buf, SOCKADDR_IN& addr, int& addrlen);
  /*关闭*/
  int close(SOCKET& sock, SOCKADDR_IN& addr, int& addrlen);

 private:
  state curr_state;       // 当前状态
  tiny_timer timer;       // 定时器
  uint16_t sock_seqnum;   // 当前序号
  uint16_t windows_size;  // 窗口大小
};