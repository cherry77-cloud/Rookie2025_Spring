/**
 * 守护进程创建函数 (Daemon Process Creation)
 *
 * 功能说明：
 * - 将当前进程转换为守护进程
 * - 完成标准守护进程创建的所有步骤
 * - 包含错误处理和资源清理
 *
 * 实现步骤：
 * 1. 创建子进程，父进程退出
 * 2. 重设文件创建掩码
 * 3. 创建新会话
 * 4. 改变工作目录到根目录
 * 5. 关闭所有文件描述符
 * 6. 重定向标准输入输出到 /dev/null
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* 守护进程函数 */
bool daemonize() {
    // 1. 创建子进程，父进程退出
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    } else if (pid > 0) {
        exit(0);
    }

    // 2. 重设文件创建掩码
    umask(0);

    // 3. 创建新会话
    pid_t sid = setsid();
    if (sid < 0) {
        return false;
    }

    // 4. 改变工作目录到根目录
    if ((chdir("/")) < 0) {
        /* Log the failure */
        return false;
    }

    // 5. 关闭所有标准文件描述符
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // 6. 重定向标准输入输出到 /dev/null
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
    return true;
}

/* 测试用守护进程的工作函数 */
void daemon_work() 
{
    FILE* log_file = fopen("/tmp/daemon.log", "a");
    if (log_file == NULL) {
        return;
    }

    for (int i = 0; i < 10; i++) {
        time_t now = time(NULL);
        fprintf(log_file, "Daemon alive at: %s", ctime(&now));
        fflush(log_file);
        sleep(1);
    }

    fclose(log_file);
}

int main() 
{
    printf("Starting daemon process test...\n");
    printf("Will write 10 timestamps to /tmp/daemon.log\n");
    printf("Use 'tail -f /tmp/daemon.log' to monitor the output\n");

    if (!daemonize()) {
        fprintf(stderr, "Failed to create daemon\n");
        return 1;
    }

    daemon_work();
    return 0;
}
