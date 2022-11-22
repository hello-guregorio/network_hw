#include "rdt_socket.h"

#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include "helper.h"
#include "packet.h"

// ����ʱ����,���������client_isn��server_isn
std::default_random_engine random_engine;
std::uniform_int_distribution<uint16_t> ud(0, 1000);

std::ostream& operator<<(std::ostream& out, state stat) {
  switch (stat) {
    case CLOSED:
      out << "CLOSED";
      break;
    case SYN_SENT:
      out << "SYN_SENT";
      break;
    case ESTABLISHED:
      out << "ESTABLISHED";
      break;
    case FIN_WAIT_1:
      out << "FIN_WAIT_1";
      break;
    case FIN_WAIT_2:
      out << "FIN_WAIT_2";
      break;
    case TIME_WAIT:
      out << "TIME_WAIT";
      break;
    case LISTEN:
      out << "LISTEN";
      break;
    case SYN_RCVD:
      out << "SYN_RCVD";
      break;
    case CLOSE_WAIT:
      out << "CLOSE_WAIT";
      break;
    case LAST_ACK:
      out << "LAST_ACK";
      break;
  }
  return out;
}
std::ostream& operator<<(std::ostream& out, header hdr) {
  out << "seq_num: " << hdr.seq_num;
  out << " ack_num: " << hdr.ack_num;
  out << " cksum: " << hdr.cksum;
  out << " flag: " << hdr.flags;
  out << " data_size: " << hdr.datalen;
  return out;
}

rdt_socket::rdt_socket()
    : curr_state(CLOSED), sock_seqnum(0), windows_size(1) {}

int rdt_socket::connect(SOCKET& sock, SOCKADDR_IN& addr, int& addrlen) {
  // Ҫ���͵�
  auto send_buffer(std::make_unique<header>());
  auto client_isn = ud(random_engine);
  sock_seqnum = client_isn;
  send_buffer->set_hdr(client_isn, 0, SYN, 0);

  // Ҫ���յ�
  auto recv_buffer(std::make_unique<header>());

  // ���Դ���
  int attempts = 0;
  while (true) {
    std::cout << "attempts: " << attempts << " curr state: " << curr_state
              << std::endl;
    if (attempts == MAX_ATTEMPTS) {
      std::cout << "exceeding max attempts" << std::endl;
      curr_state = CLOSED;
      return -1;
    }
    switch (curr_state) {
      case CLOSED: {
        sendto(sock, (char*)send_buffer.get(), sizeof(header), 0,
               (sockaddr*)&addr, addrlen);
        curr_state = SYN_SENT;
        break;
      }
      case SYN_SENT: {
        timer.restart(quick_pow(2, attempts));

        while (recvfrom(sock, (char*)recv_buffer.get(), sizeof(header), 0,
                        (sockaddr*)&addr, &addrlen) == -1) {
          if (timer.time_out()) {
            ++attempts;
            curr_state = CLOSED;
            break;
          }
        }

        if (curr_state != CLOSED) {
          if (recv_buffer->flags == SYN_ACK &&
              recv_buffer->ack_num == send_buffer->seq_num + 1 &&
              checksum((uint16_t*)recv_buffer.get(),
                       sizeof(header) / sizeof(uint16_t)) == 0) {
            attempts = 0;
            send_buffer->set_hdr(++sock_seqnum, recv_buffer->seq_num + 1, ACK,
                                 0);
            sendto(sock, (char*)send_buffer.get(), sizeof(header), 0,
                   (sockaddr*)&addr, addrlen);
            curr_state = ESTABLISHED;
          } else {
            curr_state = CLOSED;
          }
        }
        break;
      }
      case ESTABLISHED: {
        std::cout << "establish connect complete" << std::endl;
        return 1;
      }
      default: {
        std::cout << "invalid state" << std::endl;
        return -1;
      }
    }
  }
}

int rdt_socket::accept(SOCKET& sock, SOCKADDR_IN& addr, int& addrlen) {
  // server_isn
  auto server_isn = ud(random_engine);
  sock_seqnum = server_isn;
  // Ҫ���͵�
  auto send_buffer(std::make_unique<header>());
  // Ҫ���յ�
  auto recv_buffer(std::make_unique<header>());

  while (true) {
    std::cout << "curr state: " << curr_state << std::endl;
    switch (curr_state) {
      case CLOSED: {
        curr_state = LISTEN;
        break;
      }
      case LISTEN: {
        timer.restart(10);
        while (recvfrom(sock, (char*)recv_buffer.get(), sizeof(header), 0,
                        (sockaddr*)&addr, &addrlen) == -1) {
          if (timer.time_out()) {
            std::cout << "died connect" << std::endl;
            return -1;
          }
        }
        if (recv_buffer->flags == SYN &&
            checksum((uint16_t*)recv_buffer.get(),
                     sizeof(header) / sizeof(uint16_t)) == 0) {
          send_buffer->set_hdr(server_isn, recv_buffer->seq_num + 1, SYN_ACK,
                               0);
          sendto(sock, (char*)send_buffer.get(), sizeof(header), 0,
                 (sockaddr*)&addr, addrlen);
          curr_state = SYN_RCVD;
        } else {
          return -1;
        }
        break;
      }
      case SYN_RCVD: {
        timer.restart(2);
        while (recvfrom(sock, (char*)recv_buffer.get(), sizeof(header), 0,
                        (sockaddr*)&addr, &addrlen) == -1) {
          if (timer.time_out()) {
            std::cout << "cur state is SYN_RCVD" << std::endl;
            break;
          }
        }
        if (!timer.time_out()) {
          if (recv_buffer->flags == ACK &&
              recv_buffer->ack_num == send_buffer->seq_num + 1 &&
              recv_buffer->seq_num == send_buffer->ack_num &&
              checksum((uint16_t*)recv_buffer.get(),
                       sizeof(header) / sizeof(uint16_t)) == 0) {
            ++sock_seqnum;
            curr_state = ESTABLISHED;
          } else {
            return -1;
          }
        }
        break;
      }
      case ESTABLISHED: {
        std::cout << "establish connect complete" << std::endl;
        return 1;
      }
      default:
        return -1;
    }
  }
}

// �ְ��õĸ�������
static std::vector<std::unique_ptr<packet>> get_packet(const char* buf,
                                                       size_t len) {
  size_t counter = (len + MSS - 1) / MSS;
  std::vector<std::unique_ptr<packet>> ret;

  for (int i = 0; i < counter; ++i) {
    // ��ǰ�������ݶγ���
    size_t datalen = len % MSS && i == counter - 1 ? len % MSS : MSS;

    ret.emplace_back(std::make_unique<packet>());
    ret.back()->hdr.seq_num = i;

    if (i != counter - 1) {
      ret.back()->hdr.flags = DATA;
    } else {
      ret.back()->hdr.flags = TRANS_OVER;
    }

    memcpy(ret.back()->data, buf + i * MSS, datalen);
    ret.back()->hdr.datalen = datalen;
    ret.back()->hdr.cksum =
        checksum((uint16_t*)ret.back().get(), sizeof(packet) / 2);
  }
  return ret;
}

int rdt_socket::send(SOCKET& sock, char* buf, size_t len, SOCKADDR_IN& addr,
                     int& addrlen) {
  if (curr_state != ESTABLISHED) return -1;

  // ������
  int packet_num = (len + MSS - 1) / MSS;

  // �ְ�
  auto packets = get_packet(buf, len);
  std::cout << "�ְ����,����" << packet_num << "����" << std::endl;

  // recv_buffer
  auto recv_buffer(std::make_unique<header>());

  // �õ���״̬
  enum gbn { SEND, TIMEOUT, RECV } gbn_state = SEND;
  // base ������
  uint16_t base = 0;
  uint16_t nextseqnum = 0;

  // ��һ�´���ʱ�䣬��ʼ��ʱ
  std::chrono::time_point<std::chrono::high_resolution_clock> start =
      std::chrono::high_resolution_clock::now();

  while (nextseqnum < packet_num) {
    switch (gbn_state) {
      case SEND: {
        // �Ѵ���İ����ȥ
        for (nextseqnum;
             nextseqnum < packet_num && nextseqnum < base + windows_size;
             ++nextseqnum) {
          sendto(sock, (char*)packets[nextseqnum].get(), sizeof(packet), 0,
                 (sockaddr*)&addr, addrlen);
          // ��ӡ�°�����Ϣ
          std::cout << "send: " << packets[nextseqnum]->hdr << std::endl;
          if (base == nextseqnum) timer.restart(1);
        }
        gbn_state = RECV;
        break;
      }
      case TIMEOUT: {
        for (int i = base; i < nextseqnum; ++i) {
          sendto(sock, (char*)packets[i].get(), sizeof(packet), 0,
                 (sockaddr*)&addr, addrlen);
          std::cout << "resend: " << packets[i]->hdr << std::endl;
        }
        timer.restart(1);
        gbn_state = RECV;
        break;
      }
      case RECV: {
        // �жϳ�ʱ
        if (timer.time_out()) {
          gbn_state = TIMEOUT;
          break;
        }
        // û�г�ʱ������base
        if (recvfrom(sock, (char*)recv_buffer.get(), sizeof(header), 0,
                     (sockaddr*)&addr, &addrlen) != -1) {
          // �͵����������й�
          if (recv_buffer->flags == RST) {
            std::cout << "connect not established!" << std::endl;
            curr_state = CLOSED;
            return -1;
          }
          // ���յ�δ�𻵵İ�
          if (checksum((uint16_t*)recv_buffer.get(), sizeof(header) / 2) == 0) {
            base = recv_buffer->ack_num + 1;
            if (base != nextseqnum)
              timer.restart(1);
            else
              gbn_state = SEND;
          }
        }
        break;
      }
    }
  }

  // ��¼����
  std::chrono::time_point<std::chrono::high_resolution_clock> end =
      std::chrono::high_resolution_clock::now();
  ;

  std::cout
      << "�������,��ʱ"
      << std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
      << "s" << std::endl;
  return len;
}

int rdt_socket::recv(SOCKET& sock, char* buf, SOCKADDR_IN& addr, int& addrlen) {
  // �������������
  if (curr_state == SYN_RCVD) {
    auto recv_buffer(std::make_unique<packet>());
    auto send_buffer(std::make_unique<header>());
    while (recvfrom(sock, (char*)recv_buffer.get(), sizeof(packet), 0,
                    (sockaddr*)&addr, &addrlen) == -1) {
    };
    send_buffer->set_hdr(sock_seqnum, 0, RST, 0);
    sendto(sock, (char*)send_buffer.get(), sizeof(header), 0, (sockaddr*)&addr,
           addrlen);
    std::cout << "connect not established!" << std::endl;
    curr_state = CLOSED;
    return -1;
  }
  if (curr_state != ESTABLISHED) {
    std::cout << "connect not established!" << std::endl;
    return -1;
  }
  // �յ��˶����ֽ�����
  int bytecount = 0;

  auto recv_buffer(std::make_unique<packet>());

  // ��ʼ��
  auto send_buffer(std::make_unique<header>());
  send_buffer->set_hdr(0, -1, ACK, 0);

  // �൱��ack
  int expectseqnum = 0;

  do {
    if (recvfrom(sock, (char*)recv_buffer.get(), sizeof(packet), 0,
                 (sockaddr*)&addr, &addrlen) != -1) {
      // ��ӡ���յ��İ���Ϣ
      std::cout << "received: " << recv_buffer->hdr << std::endl;

      // ����յ�����Ҫ�İ���ִ�����
      if (recv_buffer->hdr.seq_num == expectseqnum &&
          checksum((uint16_t*)recv_buffer.get(), sizeof(packet) / 2) == 0) {
        memcpy(buf + expectseqnum * MSS, recv_buffer->data,
               recv_buffer->hdr.datalen);
        bytecount += recv_buffer->hdr.datalen;
        send_buffer->set_hdr(0, expectseqnum, ACK, 0);
        ++expectseqnum;
      }

      // �յ�����Ҫ����ack����ack_num�ÿ�����
      sendto(sock, (char*)send_buffer.get(), sizeof(header), 0,
             (sockaddr*)&addr, addrlen);
    }
  } while (recv_buffer->hdr.flags != TRANS_OVER);

  std::cout << "�ļ��������" << std::endl;
  return bytecount;
}

int rdt_socket::close(SOCKET& sock, SOCKADDR_IN& addr, int& addrlen) {
  return 0;
}
