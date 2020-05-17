#!/usr/bin/env python3

import socket

PORT = 2048    # Port to listen on (non-privileged ports are > 1023)
VERSION = "NextSync1"

print("NextSync server, protocol version "+VERSION)
print("by Jari Komppa 2020")
print()
hostinfo = socket.gethostbyname_ex(socket.gethostname())    
print("Running on host:", hostinfo[0])    
if hostinfo[1] != []:
    print("Aliases:")
    for x in hostinfo[1]:
        print("    " + x)
if hostinfo[2] != []:
    print("IP addresses:")
    for x in hostinfo[2]:
        print("    " + x)
        
print()
while True:
    print("NextSync listening to port", PORT)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("", PORT))
        s.listen()
        conn, addr = s.accept()
        f = 2
        with conn:
            print('Connected by', addr)
            working = True
            while working:
                data = conn.recv(1024)
                if not data:
                    break
                decoded = data.decode()
                print('Data received: "' + decoded + '",',len(decoded), 'bytes')
                if data == b"Sync":
                    print('Sending "'+VERSION+'"')
                    conn.sendall(str.encode(VERSION))
                else:
                    if data == b"Next":
                        if f == 0:
                            print("Nothing to sync")
                            conn.sendall(b'\x00\x00\x00') # end of.
                        else:
                            print("testfiles..")
                            f = f - 1;
                            conn.sendall(b'\x12\x34\x10TestFilename.duh')
                    else:
                        print("Unknown command")
                        conn.sendall(str.encode("Error"))
    print("Disconnected")
    print()
