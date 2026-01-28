#!/bin/bash

# Test 1: Real chunked transfer encoding without Content-Length
echo "🧪 Testing chunked transfer encoding..."
echo "📡 Sending chunked request to server..."

# Create chunked request manually
printf "POST /uploads/ HTTP/1.1\r\n"
printf "Host: localhost:8088\r\n"
printf "Transfer-Encoding: chunked\r\n"
printf "Content-Type: text/plain\r\n"
printf "\r\n"
printf "7\r\n"
printf "testing\r\n"
printf "A\r\n"
printf "more data!\r\n"  
printf "0\r\n"
printf "\r\n" | nc localhost 8088

echo ""
echo "✅ Chunked request sent!"
echo ""
echo "🧪 Testing normal Content-Length request for comparison..."

# Test 2: Normal Content-Length request
printf "POST /uploads/ HTTP/1.1\r\n"
printf "Host: localhost:8088\r\n"
printf "Content-Length: 17\r\n"
printf "Content-Type: text/plain\r\n"
printf "\r\n"
printf "normal test data" | nc localhost 8088

echo ""
echo "✅ Normal request sent!"
