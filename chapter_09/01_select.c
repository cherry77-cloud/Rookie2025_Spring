/**
 * TCP 带外数据处理服务器 (TCP Out-of-Band Data Server)
 *
 * 功能说明：
 * - 创建 TCP 服务器监听客户端连接
 * - 使用 select() 多路复用同时处理普通数据和带外数据
 * - 演示 TCP 套接字的带外数据传输机制
 * - 支持 SO_OOBINLINE 选项处理紧急数据
 *
 * 技术特点：
 * - 使用 select() 进行 I/O 多路复用
 * - 处理 TCP 的带外数据 (Out-of-Band Data)
 * - 分别处理普通数据和紧急数据
 * - 实时显示接收到的数据类型和内容
 * - 使用 SO_OOBINLINE 选项处理紧急数据
 * - 使用 MSG_OOB 标志接收带外数据
 */

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
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char* argv[]) 
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    // 解析命令行参数
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    printf("ip is %s and port is %d\n", ip, port);

    int ret = 0;

    // 配置服务器地址结构
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;               // IPv4协议族
    inet_pton(AF_INET, ip, &address.sin_addr);  // 转换IP地址
    address.sin_port = htons(port);             // 转换端口号

    // 创建监听套接字
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // 绑定地址到套接字
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // 开始监听连接请求
    ret = listen(listenfd, 5);
    assert(ret != -1);

    // 等待并接受客户端连接
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
        close(listenfd);
    }

    // 显示客户端连接信息
    char remote_addr[INET_ADDRSTRLEN];
    printf("connected with ip: %s and port: %d\n",
           inet_ntop(AF_INET, &client_address.sin_addr, remote_addr, INET_ADDRSTRLEN),
           ntohs(client_address.sin_port));

    // 准备数据缓冲区和文件描述符集合
    char buf[1024];
    fd_set read_fds;       // 普通数据读取集合
    fd_set exception_fds;  // 异常数据(带外数据)集合

    // 初始化文件描述符集合
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    // 设置SO_OOBINLINE选项，允许带外数据在普通数据流中接收
    int nReuseAddr = 1;
    setsockopt(connfd, SOL_SOCKET, SO_OOBINLINE, &nReuseAddr, sizeof(nReuseAddr));

    // 主要的数据处理循环
    while (1) {
        // 清空缓冲区
        memset(buf, '\0', sizeof(buf));

        // 设置要监听的文件描述符
        FD_SET(connfd, &read_fds);       // 监听普通数据
        FD_SET(connfd, &exception_fds);  // 监听带外数据

        // 使用select监听文件描述符状态变化
        ret = select(connfd + 1, &read_fds, NULL, &exception_fds, NULL);
        printf("select one\n");

        if (ret < 0) {
            printf("selection failure\n");
            break;
        }

        // 检查是否有普通数据可读
        if (FD_ISSET(connfd, &read_fds)) {
            ret = recv(connfd, buf, sizeof(buf) - 1, 0);
            if (ret <= 0) {
                break;
            }
            printf("get %d bytes of normal data: %s\n", ret, buf);
        }
        // 检查是否有带外数据可读
        else if (FD_ISSET(connfd, &exception_fds)) {
            ret = recv(connfd, buf, sizeof(buf) - 1, MSG_OOB);
            if (ret <= 0) {
                break;
            }
            printf("get %d bytes of oob data: %s\n", ret, buf);
        }
    }

    // 清理资源
    close(connfd);
    close(listenfd);
    return 0;
}
