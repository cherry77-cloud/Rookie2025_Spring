/*
 * EPOLLONESHOT示例程序
 * 功能：
 * 1. 使用epoll的EPOLLONESHOT事件实现多线程服务器
 * 2. 每个socket在任意时刻都只被一个线程处理
 * 3. 当一个线程处理完socket时，需要重置EPOLLONESHOT事件
 * 4. 支持多个客户端同时连接，每个客户端的数据由独立线程处理
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 1024

// 定义文件描述符结构体
struct fds {
    int epollfd;
    int sockfd;
};

int setnonblocking(int fd) 
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd, bool oneshot) 
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void reset_oneshot(int epollfd, int fd) 
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void* worker(void* arg) 
{
    int sockfd = ((struct fds*)arg)->sockfd;
    int epollfd = ((struct fds*)arg)->epollfd;
    printf("start new thread to receive data on fd: %d\n", sockfd);
    char buf[BUFFER_SIZE];
    memset(buf, '\0', BUFFER_SIZE);
    while (1) {
        int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
        if (ret == 0) {
            close(sockfd);
            printf("foreiner closed the connection\n");
            break;
        } else if (ret < 0) {
            if (errno == EAGAIN) {
                reset_oneshot(epollfd, sockfd);
                printf("read later\n");
                break;
            }
        } else {
            printf("get content: %s\n", buf);
            sleep(5);
        }
    }
    printf("end thread receiving data on fd: %d\n", sockfd);
}

int main(int argc, char* argv[]) 
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // 添加SO_REUSEADDR选项
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    if (ret == -1) {
        printf("Error binding socket: %s\n", strerror(errno));
        close(listenfd);
        return 1;
    }

    ret = listen(listenfd, 5);
    if (ret == -1) {
        printf("Error listening on socket: %s\n", strerror(errno));
        close(listenfd);
        return 1;
    }

    struct epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, false);

    while (1) {
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (ret < 0) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < ret; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd =
                    accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
                struct fds fds_for_new_worker;
                fds_for_new_worker.epollfd = epollfd;
                fds_for_new_worker.sockfd = connfd;
                addfd(epollfd, connfd, true);
            } else if (events[i].events & EPOLLIN) {
                pthread_t thread;
                struct fds fds_for_new_worker;
                fds_for_new_worker.epollfd = epollfd;
                fds_for_new_worker.sockfd = sockfd;
                pthread_create(&thread, NULL, worker, (void*)&fds_for_new_worker);
            } else {
                printf("something else happened \n");
            }
        }
    }

    close(listenfd);
    return 0;
}
