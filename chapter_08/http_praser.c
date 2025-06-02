/**
 * HTTP 请求解析服务器 (HTTP Request Parser Server)
 *
 * 功能说明：
 * - 创建 TCP 服务器监听 HTTP 请求
 * - 解析 HTTP 请求的请求行和头部信息
 * - 支持 GET 方法和 HTTP/1.1 协议
 * - 使用状态机模式进行请求解析
 *
 * 技术特点：
 * - 有限状态机实现：请求行解析 -> 头部解析 -> 内容解析
 * - 逐行解析 HTTP 请求内容
 * - 支持标准 HTTP 请求格式验证
 * - 错误处理和协议兼容性检查
 *
 * 解析流程：
 * 1. 接收客户端连接
 * 2. 读取请求数据到缓冲区
 * 3. 使用状态机逐行解析请求
 * 4. 验证请求方法、URL 和协议版本
 * 5. 解析请求头部信息
 * 6. 发送响应结果
 *
 * 支持的 HTTP 特性：
 * - GET 方法
 * - HTTP/1.1 协议
 * - Host 头部解析
 * - URL 路径验证
 *
 * 使用方法：
 * ./程序名 ip_address port_number
 *
 * 示例：
 * ./http_parser 127.0.0.1 8080
 * 然后使用 curl 测试：curl http://127.0.0.1:8080/
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
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

/* HTTP 解析状态枚举 */
enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };

/* 行解析状态枚举 */
enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

/* HTTP 处理结果枚举 */
enum HTTP_CODE {
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    FORBIDDEN_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
};

/* 响应消息数组 */
static const char* szret[] = {"I get a correct result\n", "Something wrong\n"};

/* 解析单行数据函数 */
enum LINE_STATUS parse_line(char* buffer, int* checked_index, int* read_index) 
{
    char temp;
    for (; *checked_index < *read_index; ++(*checked_index)) {
        temp = buffer[*checked_index];
        if (temp == '\r') {
            if ((*checked_index + 1) == *read_index) {
                return LINE_OPEN;
            } else if (buffer[*checked_index + 1] == '\n') {
                buffer[(*checked_index)++] = '\0';
                buffer[(*checked_index)++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (temp == '\n') {
            if ((*checked_index > 1) && buffer[*checked_index - 1] == '\r') {
                buffer[*checked_index - 1] = '\0';
                buffer[(*checked_index)++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

/* 解析 HTTP 请求行函数 */
enum HTTP_CODE parse_requestline(char* szTemp, enum CHECK_STATE* checkstate) 
{
    // 查找请求方法和URL之间的分隔符
    char* szURL = strpbrk(szTemp, " \t");
    if (!szURL) {
        return BAD_REQUEST;
    }
    *szURL++ = '\0';

    // 检查请求方法
    char* szMethod = szTemp;
    if (strcasecmp(szMethod, "GET") == 0) {
        printf("The request method is GET\n");
    } else {
        return BAD_REQUEST;
    }

    // 跳过空白字符
    szURL += strspn(szURL, " \t");

    // 查找URL和协议版本之间的分隔符
    char* szVersion = strpbrk(szURL, " \t");
    if (!szVersion) {
        return BAD_REQUEST;
    }
    *szVersion++ = '\0';
    szVersion += strspn(szVersion, " \t");

    // 检查协议版本
    if (strcasecmp(szVersion, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }

    // 处理完整URL（包含http://前缀的情况）
    if (strncasecmp(szURL, "http://", 7) == 0) {
        szURL += 7;
        szURL = strchr(szURL, '/');
    }

    // 验证URL格式
    if (!szURL || szURL[0] != '/') {
        return BAD_REQUEST;
    }

    printf("The request URL is: %s\n", szURL);
    *checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

/* 解析 HTTP 头部函数 */
enum HTTP_CODE parse_headers(char* szTemp) 
{
    // 空行表示头部结束
    if (szTemp[0] == '\0') {
        return GET_REQUEST;
    } else if (strncasecmp(szTemp, "Host:", 5) == 0) {
        // 解析Host头部
        szTemp += 5;
        szTemp += strspn(szTemp, " \t");
        printf("the request host is: %s\n", szTemp);
    } else {
        printf("I can not handle this header\n");
    }

    return NO_REQUEST;
}

/* 主要的HTTP内容解析函数 */
enum HTTP_CODE parse_content(char* buffer, int* checked_index, enum CHECK_STATE* checkstate,
                             int* read_index, int* start_line) 
{
    enum LINE_STATUS linestatus = LINE_OK;
    enum HTTP_CODE retcode = NO_REQUEST;

    while ((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK) {
        char* szTemp = buffer + *start_line;
        *start_line = *checked_index;

        switch (*checkstate) {
            case CHECK_STATE_REQUESTLINE: {
                retcode = parse_requestline(szTemp, checkstate);
                if (retcode == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                retcode = parse_headers(szTemp);
                if (retcode == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if (retcode == GET_REQUEST) {
                    return GET_REQUEST;
                }
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
    }

    if (linestatus == LINE_OPEN) {
        return NO_REQUEST;
    } else {
        return BAD_REQUEST;
    }
}

/* 主函数 */
int main(int argc, char* argv[]) 
{
    // 检查命令行参数
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // 配置服务器地址
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // 创建监听socket
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // 绑定地址
    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // 开始监听
    ret = listen(listenfd, 5);
    assert(ret != -1);

    printf("HTTP Parser Server listening on %s:%d\n", ip, port);

    // 接受客户端连接
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int fd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);

    if (fd < 0) {
        printf("errno is: %d\n", errno);
    } else {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        int data_read = 0;
        int read_index = 0;
        int checked_index = 0;
        int start_line = 0;
        enum CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;

        // 处理HTTP请求
        while (1) {
            data_read = recv(fd, buffer + read_index, BUFFER_SIZE - read_index, 0);
            if (data_read == -1) {
                printf("reading failed\n");
                break;
            } else if (data_read == 0) {
                printf("remote client has closed the connection\n");
                break;
            }

            read_index += data_read;
            enum HTTP_CODE result =
                parse_content(buffer, &checked_index, &checkstate, &read_index, &start_line);
            if (result == NO_REQUEST) {
                continue;
            } else if (result == GET_REQUEST) {
                send(fd, szret[0], strlen(szret[0]), 0);
                break;
            } else {
                send(fd, szret[1], strlen(szret[1]), 0);
                break;
            }
        }
        close(fd);
    }

    close(listenfd);
    return 0;
}
