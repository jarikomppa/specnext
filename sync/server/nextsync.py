#!/usr/bin/env python3

# Part of Jari Komppa's zx spectrum next suite 
# https://github.com/jarikomppa/specnext
# released under the unlicense, see http://unlicense.org 
# (practically public domain) 

import random

import datetime
import fnmatch
import socket
import struct
import time
import glob
import sys
import os

assert sys.version_info >= (3, 6) # We need 3.6 for f"" strings.

PORT = 2048    # Port to listen on (non-privileged ports are > 1023)
VERSION3 = "NextSync3"
VERSION = "NextSync4"
IGNOREFILE = "syncignore.txt"
SYNCPOINT = "syncpoint.dat"
MAX_PAYLOAD = 1024

# If you want to be really safe (but transfer slower), use this:
#MAX_PAYLOAD = 256

# The next uart has a buffer of 512 bytes; sending packets of 256 bytes will always
# fit and there won't be any buffer overruns. However, it's much slower.

opt_drive = '/'
opt_always_sync = False
opt_sync_once = False

def update_syncpoint(knownfiles):
    with open(SYNCPOINT, 'w') as f:
        for x in knownfiles:
            f.write(f"{x}\n")

def agecheck(f):
    if not os.path.isfile(SYNCPOINT):
        return False
    ptime = os.path.getmtime(SYNCPOINT)
    mtime = os.path.getmtime(f)
    if mtime > ptime:
        return False
    return True

def getFileList():    
    knownfiles = []
    if os.path.isfile(SYNCPOINT):
        with open(SYNCPOINT) as f:
            knownfiles = f.read().splitlines()
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
            if not opt_always_sync:
                if g in knownfiles:
                    if agecheck(g):
                        ignored = True
            if not ignored:
                stats = os.stat(g)
                r.append([g, stats.st_size])
    return r

def timestamp():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def sendpacket(conn, payload, packetno):
    checksum0 = 0 # random.choice([0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1]) # 5%
    checksum1 = 0
    # packetno -= random.choice([0]*99+[1]) # 1%
    for x in payload:
        checksum0 = (checksum0 ^ x) & 0xff
        checksum1 = (checksum1 + checksum0) & 0xff
    packet = ((len(payload)+5).to_bytes(2, byteorder="big")
        + payload
        + (checksum0 & 0xff).to_bytes(1, byteorder="big")
        + (checksum1 & 0xff).to_bytes(1, byteorder="big")
        + (packetno & 0xff).to_bytes(1, byteorder="big"))
    conn.sendall(packet)
    print(f'{timestamp()} | Packet sent: {len(packet)} bytes, payload: {len(payload)} bytes, checksums: {checksum0}, {checksum1}, packetno: {packetno & 0xff}')
          
def warnings():
    print()
    print(f"Note: Using {os.getcwd()} as sync root")
    if not os.path.isfile(IGNOREFILE):
        print(f"Warning! Ignore file {IGNOREFILE} not found in directory. All files will be synced, possibly including this file.")
    if not os.path.isfile(SYNCPOINT):
        print(f"Note: Sync point file {SYNCPOINT} not found, syncing all files regardless of timestamp.")
    initial = getFileList()
    total = 0
    for x in initial:
        total += x[1]
    severity = ""
    if len(initial) < 10 and total < 100000:
        severity ="Note"
    elif len(initial) < 100 and total < 1000000:
        severity = "Warning"
    else:
        severity = "WARNING"
    print(f"{severity}: Ready to sync {len(initial)} files, {total/1024:.2f} kilobytes.")
    print()

def main():
    print(f"NextSync server, protocol version {VERSION}")
    print("by Jari Komppa 2020")
    print()
    hostinfo = socket.gethostbyname_ex(socket.gethostname())    
    print(f"Running on host:\n    {hostinfo[0]}")
    if hostinfo[1] != []:
        print("Aliases:")
        for x in hostinfo[1]:
            print(f"    {x}")
    if hostinfo[2] != []:
        print("IP addresses:")
        for x in hostinfo[2]:
            print(f"    {x}")

    # If we're unsure of the ip, try getting it via internet connection
    if len(hostinfo[2]) > 1 or "127" in hostinfo[2][0]:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80)) # ping google dns
            print(f"Primary IP:\n    {s.getsockname()[0]}")
            

    warnings()
    
    working = True
    while working:
        print(f"{timestamp()} | NextSync listening to port {PORT}")
        totalbytes = 0
        payloadbytes = 0
        starttime = 0
        retries = 0
        packets = 0
        restarts = 0
        gee = 0        
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.bind(("", PORT))
            s.listen()
            conn, addr = s.accept()
            # Make sure *nixes close the socket when we ask it to.
            conn.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
            f = getFileList()
            print(f'{timestamp()} | Sync file list has {len(f)} files.')
            knownfiles = []
            if os.path.isfile(SYNCPOINT):
                with open(SYNCPOINT) as kf:
                    knownfiles = kf.read().splitlines()
            fn = 0
            filedata = b''
            packet = b''
            fileofs = 0
            totalbytes = 0
            packetno = 0
            starttime = time.time()
            endtime = starttime
            with conn:                
                print(f'{timestamp()} | Connected by {addr[0]} port {addr[1]}')
                talking = True                
                while talking:
                    data = conn.recv(1024)
                    if not data:
                        break
                    decoded = data.decode()
                    print(f'{timestamp()} | Data received: "{decoded}", {len(decoded)} bytes')
                    if data == b"Sync3":
                        print(f'{timestamp()} | Sending "{VERSION3}"')
                        packet = str.encode(VERSION3)
                        sendpacket(conn, packet, 0)
                        packets += 1
                        totalbytes += len(packet)
                    elif data == b"Next" or data == b"Neex": # Really common mistransmit. Probably uart-esp..
                        if data == b"Neex":
                            gee += 1
                        if fn >= len(f):
                            print(f"{timestamp()} | Nothing (more) to sync")
                            packet = b'\x00\x00\x00\x00\x00' # end of.
                            packets += 1
                            sendpacket(conn, packet, 0)
                            totalbytes += len(packet)
                            # Sync complete, set sync point
                            update_syncpoint(knownfiles)
                        else:
                            specfn = opt_drive + f[fn][0].replace('\\','/')
                            print(f"{timestamp()} | File:{f[fn][0]} (as {specfn}) length:{f[fn][1]} bytes")
                            packet = (f[fn][1]).to_bytes(4, byteorder="big") + (len(specfn)).to_bytes(1, byteorder="big") + (specfn).encode()
                            packets += 1
                            sendpacket(conn, packet, 0)
                            totalbytes += len(packet)
                            with open(f[fn][0], 'rb') as srcfile:
                                filedata = srcfile.read()
                            payloadbytes += len(filedata)
                            if f[fn][0] not in knownfiles:
                                knownfiles.append(f[fn][0])
                            fileofs = 0
                            packetno = 0
                            fn+=1
                    elif data == b"Get" or data == b"Gee": # Really common mistransmit. Probably uart-esp..
                        bytecount = MAX_PAYLOAD
                        if bytecount + fileofs > len(filedata):
                            bytecount = len(filedata) - fileofs                        
                        packet = filedata[fileofs:fileofs+bytecount]
                        print(f"{timestamp()} | Sending {bytecount} bytes, offset {fileofs}/{len(filedata)}")
                        packets += 1
                        sendpacket(conn, packet, packetno)
                        totalbytes += len(packet)
                        fileofs += bytecount                        
                        packetno += 1
                        if data == b"Gee":
                            gee += 1
                    elif data == b"Retry":
                        retries += 1
                        print(f"{timestamp()} | Resending")
                        sendpacket(conn, packet, packetno - 1)
                    elif data == b"Restart":
                        restarts += 1
                        print(f"{timestamp()} | Restarting")
                        fileofs = 0
                        packetno = 0
                        sendpacket(conn, str.encode("Back"), 0)
                    elif data == b"Bye":
                        sendpacket(conn, str.encode("Later"), 0)
                        print(f"{timestamp()} | Closing connection")
                        talking = False
                    elif data == b"Sync2" or data == b"Sync1" or data == b"Sync":
                        packet = str.encode("Nextsync 0.8 or later needed")
                        print(f'{timestamp()} | Old version requested')
                        sendpacket(conn, packet, 0)
                        packets += 1
                        totalbytes += len(packet)
                    else:
                        print(f"{timestamp()} | Unknown command")
                        sendpacket(conn, str.encode("Error"), 0)
                endtime = time.time()
        deltatime = endtime - starttime
        print(f"{timestamp()} | {totalbytes/1024:.2f} kilobytes transferred in {deltatime:.2f} seconds, {(totalbytes/deltatime)/1024:.2f} kBps")
        print(f"{timestamp()} | {payloadbytes/1024:.2f} kilobytes payload, {(payloadbytes/deltatime)/1024:.2f} kBps effective speed")
        print(f"{timestamp()} | packets: {packets}, retries: {retries}, restarts: {restarts}, gee: {gee}")
        print(f"{timestamp()} | Disconnected")
        print()                
        if opt_sync_once:
            working = False
            

for x in sys.argv[1:]:
    if x == '-c':
        opt_drive = 'c:/'
    elif x == '-d':
        opt_drive = 'd:/'
    elif x == '-e':
        opt_drive = 'e:/'
    elif x == '-a':
        opt_always_sync = True
    elif x == '-o':
        opt_sync_once = True
    elif x == '-s':
        MAX_PAYLOAD = 256
    elif x == '-u':
        MAX_PAYLOAD = 1455
    else:
        print(f"Unknown parameter: {x}")
        print(
        """
        Run without parameters for normal action. See nextsync.txt for details.
        
        Optional parameters:
        -a - Always sync, regardless of timestamps (doesn't skip ignore file)
        -o - Sync once, then quit. Default is to keep the sync loop running.
        -s - Use safe payload size (256 bytes). Slower, but more robust. 
             Use this if you get a lot of retries.
        -u - To live on the edge, you can try to use really unsafe payload
             size (1455 bytes). Faster, but more likely to break.
        -c - Prefix filenames with c: (i.e, /dot/foo becomes c:/dot/foo)
        -d - Prefix filenames with d: (i.e, /dot/foo becomes d:/dot/foo)
        -e - Prefix filenames wieh e: (i.e, /dot/foo becomes e:/dot/foo)
        """)
        quit()
        
main()
        