I tested with all these curls

===== CHUNKED REQUEST =====

Chunked upload POST
curl -v -X POST http://localhost:8089/uploads/helloWorld.txt      -H "Transfer-Encoding: chunked"      -d "This is a chunked message."

Chunked upload with directory
Expected: error 405. cannot upload a folder
curl -v -X POST http://localhost:8089/uploads     -H "Transfer-Encoding: chunked"      -d "This is a chunked message."