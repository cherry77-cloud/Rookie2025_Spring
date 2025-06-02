/**
 * 检测系统的字节序（大端序或小端序）
 *
 * 原理：通过联合体将一个16位整数0x0102存入内存，然后检查其在内存中的存储方式：
 * - 大端序：高字节在低地址，即 0x01 在前，0x02 在后
 * - 小端序：低字节在低地址，即 0x02 在前，0x01 在后
 */

#include <stdio.h>

void byteorder()
{
    union {
        short value;
        char union_bytes[sizeof(short)];
    } test;

    test.value = 0x0102;
    if ((test.union_bytes[0] == 1) && (test.union_bytes[1] == 2)) {
        printf("big endian\n");
    } else if ((test.union_bytes[0] == 2) && (test.union_bytes[1] == 1)) {
        printf("little endian\n");
    }
}

int main() 
{
    byteorder();
    return 0;
}
