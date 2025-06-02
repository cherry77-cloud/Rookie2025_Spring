#define _GNU_SOURCE 1
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 64

int main(int argc, char* argv[]) 
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error creating socket: %s\n", strerror(errno));
        return 1;
    }

    if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        printf("Connection failed: %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    if (ret < 0) {
        printf("Error creating pipe: %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }

    printf("Connected to server. Type your message and press Enter.\n");

    while (1) {
        ret = poll(fds, 2, -1);
        if (ret < 0) {
            printf("Poll failure: %s\n", strerror(errno));
            break;
        }

        if (fds[1].revents & POLLRDHUP) {
            printf("Server closed the connection\n");
            break;
        } else if (fds[1].revents & POLLIN) {
            memset(read_buf, '\0', BUFFER_SIZE);
            ret = recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);
            if (ret < 0) {
                printf("Error receiving data: %s\n", strerror(errno));
                break;
            } else if (ret == 0) {
                printf("Server closed connection\n");
                break;
            }
            printf("Received: %s\n", read_buf);
        }

        if (fds[0].revents & POLLIN) {
            ret = splice(STDIN_FILENO, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            if (ret < 0) {
                printf("Error reading from stdin: %s\n", strerror(errno));
                break;
            }
            ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            if (ret < 0) {
                printf("Error sending data: %s\n", strerror(errno));
                break;
            }
        }
    }

    close(pipefd[0]);
    close(pipefd[1]);
    close(sockfd);
    return 0;
}
