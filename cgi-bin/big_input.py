#!/usr/bin/env python3
import sys

def main():
    # 1. Print the CGI Header first
    # This allows your procCgiOutput() to find the content-type
    print("Content-Type: text/plain\r\n\r\n", end="")

    # 2. Print 100,000 'A' characters
    # This is roughly 100KB of data
    sys.stdout.write("A" * 100000)
    
    # 3. Flush to ensure all data is sent into the pipe
    sys.stdout.flush()

if __name__ == "__main__":
    main()