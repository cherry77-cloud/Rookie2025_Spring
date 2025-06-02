#!/bin/bash

echo "Starting HTTP Parser Server test..."

# 编译程序
echo "Compiling HTTP parser..."
gcc -o http_parser http_parser.c
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi
echo "Compilation successful!"

# 启动服务器
echo "Starting server..."
./http_parser 127.0.0.1 8080 &
SERVER_PID=$!
sleep 2

echo "Server started with PID: $SERVER_PID"

# 使用printf创建正确的HTTP请求（带\r\n）
echo "Sending test request..."
printf "GET /test HTTP/1.1\r\nHost: 127.0.0.1:8080\r\n\r\n" | nc 127.0.0.1 8080

# 清理
echo "Cleaning up..."
kill $SERVER_PID 2>/dev/null

echo "Test completed." 
