import socket
import time

s = socket.create_connection(("127.0.0.1", 9090))

s.sendall(
    b"POST /saveData HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"Content-Type: application/x-www-form-urlencoded\r\n"
    b"Content-Length: 16\r\n"
    b"\r\n"
    b"name=test&"
)

time.sleep(0.5)

s.sendall(b"value=123")

response = b""
while True:
    chunk = s.recv(4096)
    if not chunk:
        break
    response += chunk
    if b"\r\n\r\n" in response and b"303 See Other" in response:
        break

print(response.decode("latin-1"))
s.close()