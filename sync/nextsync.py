#!/usr/bin/env python3

import socket

HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
PORT = 2048    # Port to listen on (non-privileged ports are > 1023)

print("Listening to port ", PORT)
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind(("", PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print('Connected by', addr)
        while True:
            data = conn.recv(1024)
            if not data:
                break
            print('Data received: ', data.decode())
            conn.sendall(str.encode("I hear and obey\n"))
