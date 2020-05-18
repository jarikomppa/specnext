#!/usr/bin/env python3

import datetime
import fnmatch
import socket
import glob
import os

PORT = 2048    # Port to listen on (non-privileged ports are > 1023)
VERSION = "NextSync1"
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
    gf = glob.glob("*")
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
if not os.path.isfile(SYNCPOINT):
    print("Note: Sync point file "+SYNCPOINT+" not found, syncing all files regardless of timestamp.")
print("Note: Ready to sync", len(getFileList()), "files.")
print()

while True:
    print(timestamp(),"| NextSync listening to port", PORT)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("", PORT))
        s.listen()
        conn, addr = s.accept()
        f = getFileList()
        print(timestamp(), '| Sync file list has', len(f), 'files.')
        fn = 0;
        filedata = b''
        fileofs = 0
        with conn:
            print(timestamp(), '| Connected by', addr[0], 'port', addr[1])
            working = True
            while working:
                data = conn.recv(1024)
                if not data:
                    break
                decoded = data.decode()
                print(timestamp(), '| Data received: "' + decoded + '",',len(decoded), 'bytes')
                if data == b"Sync":
                    print(timestamp(), '| Sending "'+VERSION+'"')
                    conn.sendall(str.encode(VERSION))
                else:
                    if data == b"Next":
                        if fn >= len(f):
                            print(timestamp(), "| Nothing (more) to sync")
                            conn.sendall(b'\x00\x00\x00\x00\x00') # end of.
                            touch(SYNCPOINT) # Sync complete, set sync point
                        else:
                            print(timestamp(), "| File:", f[fn][0], "length:",f[fn][1])
                            packet = (f[fn][1]).to_bytes(4, byteorder="big") + (len(f[fn][0])).to_bytes(1, byteorder="big") + f[fn][0].encode()
                            #print(packet)
                            conn.sendall(packet)
                            with open(f[fn][0], 'rb') as srcfile:
                                filedata = srcfile.read()
                            fileofs = 0
                            fn+=1                                                        
                    else:
                        if data == b"Get":
                            bytecount = 1024
                            if bytecount + fileofs > len(filedata):
                                bytecount = len(filedata) - fileofs
                            checksum = 0
                            for x in filedata[fileofs:fileofs+bytecount]:
                                checksum ^= x
                            packet = filedata[fileofs:fileofs+bytecount] + (checksum & 0xff).to_bytes(1, byteorder="big")
                            conn.sendall(packet)
                            print(timestamp(), "| Sending", bytecount, "bytes, offset", fileofs,"/",len(filedata), "checksum", (checksum & 0xff), "packet size", len(packet))
                            fileofs += bytecount
                        else:
                            if data == b"Retry":
                                print(timestamp(), "| Rewinding")
                                conn.sendall(str.encode("Ok"))
                                fileofs = 0
                            else:
                                print(timestamp(), "| Unknown command")
                                conn.sendall(str.encode("Error"))
    print(timestamp(), "| Disconnected")
    print()
