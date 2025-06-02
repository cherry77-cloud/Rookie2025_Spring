/**
 * 零拷贝 Echo 服务器 (Zero-Copy Echo Server)
 *
 * 功能说明：
 * - 创建 TCP 服务器并监听指定的 IP 地址和端口
 * - 使用 splice 函数实现零拷贝数据转发
 * - 将客户端发送的数据原样返回（echo）
 *
 * 技术特点：
 * - 使用 splice 系统调用，实现了零拷贝数据传输
 * - 通过管道和 splice 组合，在内核中完成数据转发
 * - 无需在用户空间和内核空间之间拷贝数据
 *
 * 使用方法：
 * ./程序名 ip_address port_number
 *
 * 参数说明：
 * - ip_address: 服务器要绑定的 IP 地址
 * - port_number: 服务器要监听的端口号
 *
 * 示例：
 * ./server 127.0.0.1 8080
 */

#define _GNU_SOURCE /* 启用 GNU 扩展特性 */
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // 创建并初始化服务器地址
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // 创建并配置 socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // 开始监听连接请求
    ret = listen(sock, 5);
    assert(ret != -1);

    // 接受客户端连接并处理数据
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
    } else {
        // 创建管道用于零拷贝数据传输
        int pipefd[2];
        ret = pipe(pipefd);
        assert(ret != -1);

        // 使用 splice 将数据从客户端 socket 复制到管道
        ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);

        // 使用 splice 将数据从管道复制回客户端 socket
        ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);

        // 关闭连接和管道
        close(pipefd[0]);
        close(pipefd[1]);
        close(connfd);
    }

    close(sock);
    return 0;
}
