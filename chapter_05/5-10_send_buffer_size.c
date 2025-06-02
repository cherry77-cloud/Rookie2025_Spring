/**
 * TCP客户端 - TCP发送缓冲区测试程序
 *
 * 功能：
 * - 允许用户设置TCP发送缓冲区的大小
 * - 显示实际设置后的缓冲区大小（系统可能会调整）
 * - 连接到服务器后发送固定大小的数据
 *
 * 使用方法：
 * ./程序名 ip_address port_number send_buffer_size
 *
 * 参数说明：
 * - ip_address: 服务器IP地址
 * - port_number: 服务器端口
 * - send_buffer_size: 要设置的发送缓冲区大小（字节）
 *
 * 示例：
 * ./client 127.0.0.1 8080 4096
 */

#include <arpa/inet.h>
#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 512

int main(int argc, char* argv[]) 
{
    if (argc <= 3) {
        printf("usage: %s ip_address port_number send_bufer_size\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int sendbuf = atoi(argv[3]);
    int len = sizeof(sendbuf);
    // 设置发送缓冲区大小
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));
    // 获取实际的发送缓冲区大小
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, (socklen_t*)&len);
    printf("the tcp send buffer size after setting is %d\n", sendbuf);

    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) != -1) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 'a', BUFFER_SIZE);
        send(sock, buffer, BUFFER_SIZE, 0);
    } else {
        printf("connection failed\n");
    }

    close(sock);
    return 0;
}
