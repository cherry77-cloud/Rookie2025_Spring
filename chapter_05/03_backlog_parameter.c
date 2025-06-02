/*
 * 简单的TCP服务器程序
 * 
 * 功能：
 * 1. 创建TCP socket并绑定到指定的IP地址和端口
 * 2. 开始监听客户端连接请求
 * 3. 支持通过SIGTERM信号优雅停止服务器
 * 4. 程序会持续运行直到收到停止信号
 * 
 * 使用方法：
 * ./program ip_address port_number backlog
 * 
 * 参数说明：
 * - ip_address: 服务器绑定的IP地址
 * - port_number: 服务器监听的端口号
 * - backlog: 监听队列的最大长度
 * 
 * 注意：此程序只创建监听socket，不处理实际的客户端连接
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <libgen.h>
#include <string.h>

static bool stop = false;
static void handle_term(int sig) {
    stop = true;
}

int main(int argc, char* argv[])
{
    signal(SIGTERM, handle_term);

    if (argc <= 3) {
        printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, backlog);
    assert(ret != -1);

    while (!stop) {
        sleep(1);
    }

    close(sock);
    return 0;
}
