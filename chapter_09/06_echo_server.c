/*
 * TCP/UDP回显服务器
 *
 * 功能：
 * 1. 同时支持TCP和UDP连接
 * 2. 使用epoll管理连接
 * 3. 将收到的数据直接回显给客户端
 *
 * 使用方法：
 * ./echo_server <ip_address> <port_number>
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENT_NUMBER 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 1024

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;  // 使用水平触发模式，更安全
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // 创建TCP socket
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("Error creating TCP socket: %s\n", strerror(errno));
        return 1;
    }

    // 设置地址重用
    int reuse = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        printf("Error setting SO_REUSEADDR: %s\n", strerror(errno));
        close(listenfd);
        return 1;
    }

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    if (ret < 0) {
        printf("Error binding TCP socket: %s\n", strerror(errno));
        close(listenfd);
        return 1;
    }

    ret = listen(listenfd, 5);
    if (ret < 0) {
        printf("Error listening on TCP socket: %s\n", strerror(errno));
        close(listenfd);
        return 1;
    }

    // 创建UDP socket
    int udpfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (udpfd < 0) {
        printf("Error creating UDP socket: %s\n", strerror(errno));
        close(listenfd);
        return 1;
    }

    ret = bind(udpfd, (struct sockaddr*)&address, sizeof(address));
    if (ret < 0) {
        printf("Error binding UDP socket: %s\n", strerror(errno));
        close(listenfd);
        close(udpfd);
        return 1;
    }

    // 创建epoll实例
    int epollfd = epoll_create(5);
    if (epollfd < 0) {
        printf("Error creating epoll instance: %s\n", strerror(errno));
        close(listenfd);
        close(udpfd);
        return 1;
    }

    struct epoll_event events[MAX_EVENT_NUMBER];
    addfd(epollfd, listenfd);
    addfd(epollfd, udpfd);

    printf("Echo server is running on %s:%d\n", ip, port);
    printf("Supporting both TCP and UDP connections\n");

    while (1) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0) {
            if (errno == EINTR) {
                continue;  // 处理信号中断
            }
            printf("epoll failure: %s\n", strerror(errno));
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                // 处理新的TCP连接
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd =
                    accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
                if (connfd < 0) {
                    printf("Error accepting connection: %s\n", strerror(errno));
                    continue;
                }
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
                printf("New TCP connection from %s:%d\n", client_ip,
                       ntohs(client_address.sin_port));
                addfd(epollfd, connfd);
            } else if (sockfd == udpfd) {
                // 处理UDP数据
                char buf[UDP_BUFFER_SIZE];
                memset(buf, '\0', UDP_BUFFER_SIZE);
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);

                ret = recvfrom(udpfd, buf, UDP_BUFFER_SIZE - 1, 0,
                               (struct sockaddr*)&client_address, &client_addrlength);
                if (ret < 0) {
                    printf("Error receiving UDP data: %s\n", strerror(errno));
                    continue;
                }
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
                printf("Received UDP data from %s:%d: %s\n", client_ip,
                       ntohs(client_address.sin_port), buf);

                ret = sendto(udpfd, buf, ret, 0, (struct sockaddr*)&client_address,
                             client_addrlength);
                if (ret < 0) {
                    printf("Error sending UDP response: %s\n", strerror(errno));
                }
            } else if (events[i].events & EPOLLIN) {
                // 处理TCP数据
                char buf[TCP_BUFFER_SIZE];
                memset(buf, '\0', TCP_BUFFER_SIZE);
                ret = recv(sockfd, buf, TCP_BUFFER_SIZE - 1, 0);
                if (ret < 0) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        continue;
                    }
                    printf("Error receiving TCP data: %s\n", strerror(errno));
                    close(sockfd);
                    continue;
                } else if (ret == 0) {
                    printf("Client closed connection\n");
                    close(sockfd);
                    continue;
                }

                printf("Received TCP data: %s\n", buf);
                ret = send(sockfd, buf, ret, 0);
                if (ret < 0) {
                    printf("Error sending TCP response: %s\n", strerror(errno));
                }
            }
        }
    }

    close(listenfd);
    close(udpfd);
    close(epollfd);
    return 0;
}
