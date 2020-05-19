#!/usr/bin/env python3

# Part of Jari Komppa's zx spectrum next suite 
# https://github.com/jarikomppa/specnext
# released under the unlicense, see http://unlicense.org 
# (practically public domain) 

import datetime
import fnmatch
import socket
import glob
import os

PORT = 2048    # Port to listen on (non-privileged ports are > 1023)
VERSION = "NextSync2"
IGNOREFILE = "syncignore.txt"
SYNCPOINT = "syncpoint.dat"

def touch(fname):
    if os.path.exists(fname):
        os.utime(fname, None)
    else:
        open(fname, 'a').close()

def agecheck(f):
    if not os.path.isfile(SYNCPOINT):
        return False
    ptime = os.path.getmtime(SYNCPOINT)
    ftime = os.path.getmtime(f)
    if ftime > ptime:
        return False
    return True

def getFileList():    
    ignorelist = []
    if os.path.isfile(IGNOREFILE):
        with open(IGNOREFILE) as f:
            ignorelist = f.read().splitlines()
    r = []
    gf = glob.glob("**", recursive=True)
    for g in gf:
        if os.path.isfile(g) and os.path.exists(g):
            ignored = False
            for i in ignorelist:
                if fnmatch.fnmatch(g, i):
                    ignored = True
            if agecheck(g):
                ignored = True
            if not ignored:
                stats = os.stat(g)
                r.append([g, stats.st_size])
    return r;

def timestamp():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def sendpacket(conn, payload):
    checksum0 = 0
    checksum1 = 0    
    for x in payload:
        checksum0 = (checksum0 ^ x) & 0xff
        checksum1 = (checksum1 + checksum0) & 0xff
    packet = ((len(payload)+4).to_bytes(2, byteorder="big")
        + payload
        + (checksum0 & 0xff).to_bytes(1, byteorder="big")
        + (checksum1 & 0xff).to_bytes(1, byteorder="big"))
    conn.sendall(packet)
    print(f'{timestamp()} | Packet sent: {len(packet)} bytes, payload: {len(payload)} bytes, checksums: {checksum0}, {checksum1}')
          

print(f"NextSync server, protocol version {VERSION}")
print("by Jari Komppa 2020")
print()
hostinfo = socket.gethostbyname_ex(socket.gethostname())    
print(f"Running on host:{hostinfo[0]}")
if hostinfo[1] != []:
    print("Aliases:")
    for x in hostinfo[1]:
        print(f"    {x}")
if hostinfo[2] != []:
    print("IP addresses:")
    for x in hostinfo[2]:
        print(f"    {x}")

print()
if not os.path.isfile(IGNOREFILE):
    print(f"Warning! Ignore file {IGNOREFILE} not found in directory. All files will be synced.")
if not os.path.isfile(SYNCPOINT):
    print(f"Note: Sync point file {SYNCPOINT} not found, syncing all files regardless of timestamp.")
print(f"Note: Ready to sync {len(getFileList())} files.")
print()

while True:
    print(f"{timestamp()} | NextSync listening to port {PORT}")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("", PORT))
        s.listen()
        conn, addr = s.accept()
        f = getFileList()
        print(f'{timestamp()} | Sync file list has {len(f)} files.')
        fn = 0;
        filedata = b''
        packet = b''
        fileofs = 0
        with conn:
            print(f'{timestamp()} | Connected by {addr[0]} port {addr[1]}')
            working = True
            while working:
                data = conn.recv(1024)
                if not data:
                    break
                decoded = data.decode()
                print(f'{timestamp()} | Data received: "{decoded}", {len(decoded)} bytes')
                if data == b"Sync2":
                    print(f'{timestamp()} | Sending "{VERSION}"')
                    sendpacket(conn, str.encode(VERSION))
                else:
                    if data == b"Next":
                        if fn >= len(f):
                            print(f"{timestamp()} | Nothing (more) to sync")
                            packet = b'\x00\x00\x00\x00\x00' # end of.
                            sendpacket(conn, packet)
                            touch(SYNCPOINT) # Sync complete, set sync point
                        else:
                            specfn = '/' + f[fn][0].replace('\\','/')
                            print(f"{timestamp()} | File:{f[fn][0]} (as {specfn}) length:{f[fn][1]} bytes")
                            packet = (f[fn][1]).to_bytes(4, byteorder="big") + (len(specfn)).to_bytes(1, byteorder="big") + (specfn).encode()
                            #print(packet)
                            sendpacket(conn,packet)
                            with open(f[fn][0], 'rb') as srcfile:
                                filedata = srcfile.read()
                            fileofs = 0
                            fn+=1                                                        
                    else:
                        if data == b"Get":
                            bytecount = 1024
                            if bytecount + fileofs > len(filedata):
                                bytecount = len(filedata) - fileofs
                            packet = filedata[fileofs:fileofs+bytecount]
                            print(f"{timestamp()} | Sending {bytecount} bytes, offset {fileofs}/{len(filedata)}")
                            sendpacket(conn, packet)
                            fileofs += bytecount
                        else:
                            if data == b"Retry":
                                print(f"{timestamp()} | Resending")
                                sendpacket(conn, packet)
                            else:
                                print(f"{timestamp()} | Unknown command")
                                sendpacket(conn,str.encode("Error"))
    print(f"{timestamp()} | Disconnected")
    print()
