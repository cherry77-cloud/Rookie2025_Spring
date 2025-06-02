/**
 * TCP服务器 - TCP接收缓冲区测试程序
 *
 * 功能：
 * - 允许用户设置TCP接收缓冲区的大小
 * - 显示实际设置后的接收缓冲区大小（系统可能会调整）
 * - 接受客户端连接并持续接收数据直到连接关闭
 * - 使用while循环持续读取数据，演示接收缓冲区的使用
 *
 * 使用方法：
 * ./程序名 ip_address port_number receive_buffer_size
 *
 * 参数说明：
 * - ip_address: 服务器要绑定的IP地址
 * - port_number: 服务器要监听的端口
 * - receive_buffer_size: 要设置的接收缓冲区大小（字节）
 *
 * 示例：
 * ./server 127.0.0.1 8080 4096
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) 
{
    if (argc <= 3) {
        printf("usage: %s ip_address port_number receive_buffer_size\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int recvbuf = atoi(argv[3]);
    int len = sizeof(recvbuf);
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, (socklen_t*)&len);
    printf("the receive buffer size after settting is %d\n", recvbuf);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
    } else {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        while (recv(connfd, buffer, BUFFER_SIZE - 1, 0) > 0) {
        }
        close(connfd);
    }

    close(sock);
    return 0;
}
