/**
 * 用户身份信息查看程序 (User Identity Information Viewer)
 *
 * 功能说明：
 * - 显示当前进程的实际用户 ID (UID)
 * - 显示当前进程的有效用户 ID (EUID)
 * 示例输出：
 * userid is 1000, effective userid is: 1000
**/

#include <stdio.h>
#include <unistd.h>

int main() 
{
    uid_t uid = getuid();
    uid_t euid = geteuid();
    printf("userid is %d, effective userid is: %d\n", uid, euid);
    return 0;
}
