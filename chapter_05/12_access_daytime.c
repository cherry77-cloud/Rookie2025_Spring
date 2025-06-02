/**
 * Daytime服务客户端程序
 *
 * 功能：
 * - 连接到指定主机的daytime服务（TCP，端口13）
 * - 获取并显示远程服务器提供的日期和时间
 * - 演示使用getservbyname和gethostbyname等网络信息查询函数
 *
 * 使用方法：
 * ./程序名 hostname
 *
 * 参数说明：
 * - hostname: 提供daytime服务的主机名
 *
 * 示例：
 * ./client time.nist.gov
 *
 * 技术说明：
 * - 使用getservbyname查询daytime服务的端口号
 * - 使用gethostbyname解析主机名到IP地址
 * - 通过TCP协议获取时间信息
 */

#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[]) 
{
    assert(argc == 2);
    char* host = argv[1];
    struct hostent* hostinfo = gethostbyname(host);
    assert(hostinfo);
    struct servent* servinfo = getservbyname("daytime", "tcp");
    assert(servinfo);
    printf("daytime port is %d\n", ntohs(servinfo->s_port));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = servinfo->s_port;
    address.sin_addr = *(struct in_addr*)*hostinfo->h_addr_list;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int result = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(result != -1);

    char buffer[128];
    result = read(sockfd, buffer, sizeof(buffer));
    assert(result > 0);
    buffer[result] = '\0';
    printf("the day tiem is: %s", buffer);
    close(sockfd);
    return 0;
}
