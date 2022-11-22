#define _WINSOCK_DEPRECATED_NO_WARNINGS 

#include "helper.h"
#include "packet.h"
#include "rdt_socket.h"
#include <iostream>
#include <fstream>
int main() {
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    //server address
    SOCKADDR_IN server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4000);
    server_addr.sin_addr.s_addr = inet_addr("10.130.78.57");
    int server_addr_len = sizeof(server_addr);

    //router address
    SOCKADDR_IN router_addr;
    router_addr.sin_family = AF_INET;
    router_addr.sin_port = htons(4001);
    router_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int router_addr_len = sizeof(router_addr);

    //server socket
    SOCKET server = socket(AF_INET, SOCK_DGRAM, 0);
    //nonblock io
    u_long mode = 1;
    ioctlsocket(server, FIONBIO, &mode);

    //bind
    if (bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr)) == -1) {
        std::cout << "binding failed!" << std::endl;
        return 0;
    }

    rdt_socket rdt;
    if (rdt.accept(server, router_addr, router_addr_len) == -1) {
        std::cout << "accept error happen!" << std::endl;
        return 0;
    }

    auto buffer(std::make_unique<char[]>(1024 * 10240));
    int recv_len = rdt.recv(server, buffer.get(), router_addr, router_addr_len);
    std::cout << recv_len <<std::endl;

    const char* file_name = "test.jpg";
    std::ofstream output(file_name,std::ofstream::binary);
    output.write(buffer.get(), recv_len);
    output.close();
}