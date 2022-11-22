#pragma once
#include <WinSock2.h>

#include <iostream>

#include "helper.h"

#pragma comment(lib, "ws2_32.lib")

#define MAX_ATTEMPTS 3  // ����Դ���
// tcp���ֻ���ʱ�ĸ���״̬,��״̬��ʵ��
enum state {
  /*�ͻ����õ���״̬*/
  CLOSED,       // �ر�          <==�����׽���ʱ��̬�����
  SYN_SENT,     // ����syn
  ESTABLISHED,  // ���ӽ���
  FIN_WAIT_1,   // ���ֵȴ�1
  FIN_WAIT_2,   // ���ֵȴ�2
  TIME_WAIT,    // �������ȴ�ʱ��
  /*������õ���״̬*/
  LISTEN,      // ����ʱ����
  SYN_RCVD,    // ����������ʱ
  CLOSE_WAIT,  // ���ֹر�
  LAST_ACK     // �����һ��ack
};
// ���������,�������
std::ostream& operator<<(std::ostream& out, state stat);

class rdt_socket {
 public:
  rdt_socket();
  /*����*/
  int connect(SOCKET& sock, SOCKADDR_IN& addr, int& addrlen);
  /*����*/
  int accept(SOCKET& sock, SOCKADDR_IN& addr, int& addrlen);
  /*send*/
  int send(SOCKET& sock, char* buf, size_t len, SOCKADDR_IN& addr,
           int& addrlen);
  /*recv*/
  int recv(SOCKET& sock, char* buf, SOCKADDR_IN& addr, int& addrlen);
  /*�ر�*/
  int close(SOCKET& sock, SOCKADDR_IN& addr, int& addrlen);

 private:
  state curr_state;       // ��ǰ״̬
  tiny_timer timer;       // ��ʱ��
  uint16_t sock_seqnum;   // ��ǰ���
  uint16_t windows_size;  // ���ڴ�С
};