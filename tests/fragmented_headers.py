import socket
import time

with socket.create_connection(("127.0.0.1", 9090)) as s:
    s.settimeout(3)

    fragments = [
        b"GET / HT",
        b"TP/1.1\r\nHo",
        b"st: localhost\r\n",
        b"Connection: close\r\n",
        b"\r\n",
    ]

    for fragment in fragments:
        s.sendall(fragment)
        time.sleep(0.1)

    response = bytearray()

    try:
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            response.extend(chunk)
    except socket.timeout:
        print("Timed out waiting for response")

    print(response.decode("utf-8", errors="replace"))