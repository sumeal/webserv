import time
import sys

# Simulate a script that takes 3 seconds to "think"
time.sleep(3) 

print("Content-Type: text/plain\r\n\r\n", end="")
print("Sorry I'm late! I was thinking for 3 seconds.")