/**
 * HTTP文件服务器
 *
 * 功能：
 * - 创建HTTP服务器，监听指定的IP地址和端口
 * - 接收HTTP请求并发送指定文件的内容
 * - 使用writev函数实现高效的集中写操作
 * - 支持基本的HTTP响应（200 OK和500错误）
 *
 * 使用方法：
 * ./程序名 ip_address port_number filename
 *
 * 参数说明：
 * - ip_address: 服务器绑定的IP地址
 * - port_number: 服务器监听的端口
 * - filename: 要传输的文件名
 *
 * 示例：
 * ./server 127.0.0.1 8080 index.html
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

/* HTTP响应状态行 */
static const char* status_line[2] = {"200 OK", "500 Internal server error"};

int main(int argc, char* argv[]) 
{
    if (argc <= 3) {
        printf("usage: %s ip_address port_number filename\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* file_name = argv[3];

    /* 创建并初始化服务器地址 */
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    /* 创建socket并绑定到服务器地址 */
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    /* 开始监听 */
    ret = listen(sock, 5);
    assert(ret != -1);

    /* 接受客户端连接 */
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
    } else {
        /* 准备HTTP响应的缓冲区 */
        char header_buf[BUFFER_SIZE];
        memset(header_buf, '\0', BUFFER_SIZE);
        char* file_buf = NULL; /* 用于存储文件内容 */
        struct stat file_stat;
        bool valid = true;
        int len = 0;

        /* 获取文件状态 */
        if (stat(file_name, &file_stat) < 0) {
            valid = false;
        } else {
            if (S_ISDIR(file_stat.st_mode)) { /* 检查是否为目录 */
                valid = false;
            } else if (file_stat.st_mode & S_IROTH) { /* 检查是否可读 */
                int fd = open(file_name, O_RDONLY);
                if (fd < 0) {
                    valid = false;
                } else {
                    /* 分配文件内容缓冲区 */
                    file_buf = (char*)malloc(file_stat.st_size + 1);
                    if (file_buf == NULL) {
                        valid = false;
                    } else {
                        memset(file_buf, '\0', file_stat.st_size + 1);
                        if (read(fd, file_buf, file_stat.st_size) < 0) {
                            valid = false;
                        }
                    }
                    close(fd);
                }
            } else {
                valid = false;
            }
        }

        /* 根据文件读取结果发送响应 */
        if (valid) {
            /* 发送成功响应 */
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "Content-Length: %ld\r\n",
                           file_stat.st_size);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");

            /* 使用writev同时发送头部和文件内容 */
            struct iovec iv[2];
            iv[0].iov_base = header_buf;
            iv[0].iov_len = strlen(header_buf);
            iv[1].iov_base = file_buf;
            iv[1].iov_len = file_stat.st_size;
            ret = writev(connfd, iv, 2);
        } else {
            /* 发送错误响应 */
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[1]);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");
            send(connfd, header_buf, strlen(header_buf), 0);
        }

        /* 清理资源 */
        if (file_buf != NULL) {
            free(file_buf);
        }
        close(connfd);
    }

    close(sock);
    return 0;
}
