/**
 * 文件传输服务器 (File Transfer Server)
 *
 * 功能说明：
 * - 创建 TCP 服务器并监听指定的 IP 地址和端口
 * - 使用 sendfile 函数实现零拷贝文件传输
 * - 支持高效的大文件传输
 *
 * 技术特点：
 * - 使用 sendfile 系统调用，避免了用户空间和内核空间的数据拷贝
 * - 直接在内核中传输文件数据，提高传输效率
 * - 简化了文件传输的实现流程
 *
 * 使用方法：
 * ./程序名 ip_address port_number filename
 *
 * 参数说明：
 * - ip_address: 服务器要绑定的 IP 地址
 * - port_number: 服务器要监听的端口号
 * - filename: 要传输的文件名
 *
 * 示例：
 * ./server 127.0.0.1 8080 test.txt
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
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv[]) 
{
    // 检查命令行参数
    if (argc <= 3) {
        printf("usage: %s ip_address port_number filename\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* file_name = argv[3];

    // 打开并获取文件信息
    int filefd = open(file_name, O_RDONLY);
    assert(filefd > 0);
    struct stat stat_buf;
    fstat(filefd, &stat_buf);

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

    // 接受客户端连接
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
    } else {
        // 使用 sendfile 直接传输文件
        sendfile(connfd, filefd, NULL, stat_buf.st_size);
        close(connfd);
    }

    close(sock);
    close(filefd);  // 关闭文件描述符
    return 0;
}
