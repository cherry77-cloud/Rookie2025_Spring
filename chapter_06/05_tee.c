/**
 * 零拷贝文件复制程序 (Zero-Copy File Copy)
 *
 * 功能说明：
 * - 从标准输入读取数据
 * - 同时将数据复制到文件和标准输出
 * - 使用 splice 和 tee 实现零拷贝数据传输
 *
 * 技术特点：
 * - 使用 splice 在文件描述符之间直接传输数据
 * - 使用 tee 在管道之间复制数据
 * - 全程在内核中完成数据传输，无需用户空间缓冲区
 *
 * 使用方法：
 * echo "some text" | ./程序名 output_file
 *
 * 参数说明：
 * - output_file: 要写入的目标文件
 *
 * 示例：
 * echo "Hello, World!" | ./copier output.txt
 */

#define _GNU_SOURCE /* 启用 GNU 扩展特性 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) 
{
    if (argc != 2) {
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }

    // 创建输出文件
    int filefd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    assert(filefd > 0);

    // 创建用于标准输出的管道
    int pipefd_stdout[2];
    int ret = pipe(pipefd_stdout);
    assert(ret != -1);

    // 创建用于文件输出的管道
    int pipefd_file[2];
    ret = pipe(pipefd_file);
    assert(ret != -1);

    // 从标准输入复制数据到第一个管道
    ret = splice(STDIN_FILENO, NULL, pipefd_stdout[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);

    // 使用 tee 将数据复制到第二个管道
    ret = tee(pipefd_stdout[0], pipefd_file[1], 32768, SPLICE_F_NONBLOCK);
    assert(ret != -1);

    // 将数据从第二个管道写入文件
    ret = splice(pipefd_file[0], NULL, filefd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);

    // 将数据从第一个管道写入标准输出
    ret = splice(pipefd_stdout[0], NULL, STDOUT_FILENO, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);

    // 清理资源
    close(filefd);
    close(pipefd_stdout[0]);
    close(pipefd_stdout[1]);
    close(pipefd_file[0]);
    close(pipefd_file[1]);
    return 0;
}
