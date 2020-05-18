#!/usr/bin/env python3

import fnmatch
import socket
import glob
import os

PORT = 2048    # Port to listen on (non-privileged ports are > 1023)
VERSION = "NextSync1"
IGNOREFILE = "syncignore.txt"


def getFileList():
    ignorelist = []
    if os.path.isfile(IGNOREFILE):
        with open(IGNOREFILE) as f:
            ignorelist = f.read().splitlines()
    r = []
    gf = glob.glob("*")
    for g in gf:        
        if os.path.isfile(g) and os.path.exists(g):
            ignored = False
            for i in ignorelist:
                if fnmatch.fnmatch(g, i):
                    ignored = True
            if not ignored:
                stats = os.stat(g)
                r.append([g, stats.st_size])
    return r;

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
if not os.path.isfile(IGNOREFILE):
    print("Warning! Ignore file "+IGNOREFILE+" not found in directory. All files will be synced.")
    print()

while True:
    print("NextSync listening to port", PORT)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("", PORT))
        s.listen()
        conn, addr = s.accept()
        f = getFileList()
        fn = 0;
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
                        if fn >= len(f):
                            print("Nothing to sync")
                            conn.sendall(b'\x00\x00\x00\x00\x00') # end of.
                        else:
                            print("file:", f[fn][0], "len:",f[fn][1])
                            packet = (f[fn][1]).to_bytes(4, byteorder="big") + (len(f[fn][0])).to_bytes(1, byteorder="big") + f[fn][0].encode()
                            #print(packet)
                            conn.sendall(packet)
                            fn+=1
                    else:
                        print("Unknown command")
                        conn.sendall(str.encode("Error"))
    print("Disconnected")
    print()
