/**
 * 用户权限切换函数 (User Privilege Switching Function)
 *
 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

/* 用户权限切换函数 */
static bool switch_to_user(uid_t user_id, gid_t gp_id) 
{
    if ((user_id == 0) && (gp_id == 0)) {
        return false;
    }

    gid_t gid = getgid();
    uid_t uid = getuid();
    if (((gid != 0) || (uid != 0)) && ((gid != gp_id) || (uid != user_id))) {
        return false;
    }

    if (uid != 0) {
        return true;
    }

    if ((setgid(gp_id) < 0) || (setuid(user_id) < 0)) {
        return false;
    }

    return true;
}

int main() 
{
    printf("Current UID: %d, GID: %d\n", getuid(), getgid());

    printf("\nTest 1: Switching to user 1000\n");
    if (switch_to_user(1000, 1000)) {
        printf("Success: Switched to user 1000\n");
        printf("New UID: %d, GID: %d\n", getuid(), getgid());
    } else {
        printf("Failed: Could not switch to user 1000\n");
    }

    printf("\nTest 2: Trying to switch to root (should fail)\n");
    if (switch_to_user(0, 0)) {
        printf("Error: Unexpectedly switched to root\n");
    } else {
        printf("Success: Prevented switching to root\n");
    }

    return 0;
}
