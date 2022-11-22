#define _WINSOCK_DEPRECATED_NO_WARNINGS 

#include "helper.h"
#include "packet.h"
#include "rdt_socket.h"
#include <fstream>
#include <iostream>
int main() {
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    //server addr
    SOCKADDR_IN server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4000);
    server_addr.sin_addr.s_addr = inet_addr("10.130.78.57");
    int server_addr_len = sizeof(server_addr);

    //router addr
    SOCKADDR_IN router_addr;
    router_addr.sin_family = AF_INET;
    router_addr.sin_port = htons(4001);
    router_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int router_addr_len = sizeof(router_addr);

    //client socket
    SOCKET client = socket(AF_INET, SOCK_DGRAM, 0);
    //nonblock io
    u_long mode = 1;
    ioctlsocket(client, FIONBIO, &mode);

    rdt_socket rdt;
    if (rdt.connect(client, router_addr, router_addr_len) == -1) {
        std::cout << "connect error happen!" << std::endl;
    }

    auto buffer(std::make_unique<char[]>(1024 * 10240));
    const char* file_name = "icon1.jpg";
    auto filesize = file_size(file_name);

    std::ifstream input(file_name, std::ifstream::in | std::ios::binary);
    input.read(buffer.get(), filesize);


    int rouaddrlen = sizeof(router_addr);
    rdt.send(client, buffer.get(), filesize, router_addr,
        rouaddrlen);

    input.close();
}