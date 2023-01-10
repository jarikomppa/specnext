#!/usr/bin/env python3

# Part of Jari Komppa's zx spectrum next suite 
# https://github.com/jarikomppa/specnext
# released under the unlicense, see http://unlicense.org 
# (practically public domain) 

import time
import glob
import sys
import os
import re

assert sys.version_info >= (3, 6) # We need 3.6 for f"" strings.

MAX_PAYLOAD = 1024

def getFileList():    
    r = []
#    gf = glob.glob("test/nextsync.lst")
    gf = glob.glob("f:/spec_archive/Games/**", recursive=True)
#    gf = glob.glob("f:/spec_archive/Games/a/*.tap", recursive=True)
#    gf = glob.glob("**", recursive=True)
#    gf = glob.glob("f:/spec_archive/Applications/123/3D Construction Kit - Editor (1991)(Domark)(128K).tap")
#    gf = glob.glob("/specnext/sync12/pack/*.tap") * 1
#    gf = glob.glob("f:/spec_archive/Applications/123/*.tap")
#    gf = glob.glob("C:/Windows/System32/**", recursive=True)
#    gf = glob.glob("C:/Windows/System32/*.exe")
#    gf = glob.glob("C:/Windows/System32/MRT.exe")

    for g in gf:
        if os.path.isfile(g) and os.path.exists(g):
            stats = os.stat(g)
            r.append([g, stats.st_size])
    return r


#Compressed bytes  :48344
def compress_py(d, fn):
    l = []
    i = 0
    ld = len(d)
    while i < ld:
        skip = i
        while skip+2 < ld and not (d[skip] == d[skip+1] and d[skip] == d[skip+2]):
            skip += 1
        if ld - skip <= 2:
            skip = ld
        skip -= i
        while skip > 8192:
            l.append(0xff)
            l.append(8192 >> 8)
            l.append(8192 & 0xff)
            for x in range(8192):
                l.append(d[i])
                i += 1
            l.append(1)
            l.append(d[i])
            i += 1
            skip -= 8192 + 1
        if skip < 0xff:
            l.append(skip)
        else:
            l.append(0xff)
            l.append(skip >> 8)
            l.append(skip & 0xff)
        for x in range(skip):
            l.append(d[i])
            i += 1
        if i < ld:
            run = i + 1
            while run < ld and d[i] == d[run]:
                run += 1
            run -= i
            if run > 8192:
                run = 8192
            if run < 0xff:
                l.append(run)
            else:
                l.append(0xff)
                l.append(run >> 8)
                l.append(run & 0xff)
            l.append(d[i])
            i += run

    return bytes(l)

# Matches runs, but outputs extra item after a run, which 
# we need to skip.. TODO: find a regex that doesn's need
# the skip.
compress_splitter = re.compile(b"((.)\\2{2,8191})",re.DOTALL)

def compress_py2(d, fn):
    l = []
    splits = compress_splitter.split(d)
    prevblock = 1
    count = 0
    skip = 0
    for x in splits:
        if skip == 0:
            lx = len(x)
            count += lx
            if lx > 0:
                if lx > 2 and x[0] == x[1] and x[0] == x[2]:
                    # run block
                    # Due to regex matching up to 8192 bytes,
                    # the runs are always suitably sized
                    if prevblock == 1:
                        l += [0] # add empty skip block
                    if lx < 0xff:
                        l += [lx]
                    else:
                        l += [0xff, lx >> 8, lx & 0xff]
                    l += [x[0]]
                    prevblock = 1
                    skip = 1
                else:
                    # skip block
                    y = x
                    if prevblock == 0:
                        l += [1, x[0]] # add 1 length run block
                        y = x[1:]
                        prevblock = 1
                    ly = len(y)
                    if ly > 0:
                        while ly > 8192:
                            # If too long block, split to 8k segs
                            l += [0xff, 8192 >> 8, 8192 & 0xff]
                            l += y[:8192]
                            y = y[8192:]
                            l += [1, y[0]] # add a 1 byte RLE
                            y = y[1:]
                            ly = len(y)

                        if ly < 0xff:
                            l += [ly]
                        else:
                            l += [0xff, ly >> 8, ly & 0xff]
                        l += y

                        prevblock = 0
        else:
            skip = 0            
    return bytes(l)

def compress_offline(d, fn):
    os.system('pack.exe "' + fn + '" packfile.temp')
    with open("packfile.temp", 'rb') as f:
        return f.read()

# 12.642497539520264
def decompress(d):
    l = []
    i = 0
    while i < len(d):
        skip = d[i]
        i += 1
        if skip == 0xff:
            skip = (d[i] << 8) + d[i + 1]
            i += 2
        l += d[i:i+skip]
        i += skip
        if i < len(d):
            run = d[i]
            i += 1
            if run == 0xff:
                run = (d[i] << 8) + d[i + 1]
                i += 2
            l += [d[i]] * run
            i += 1

    return bytes(l)

# 4.37
# 2.53
def main():
    compress = compress_py2 #offline
    filedata = b''
    compressed = b''
    totalbytes = 0
    compressedbytes = 0
    totalpackets = 0
    compressedpackets = 0
    print("Recursing...")
    rstarttime = time.time()
    filelist = getFileList()
    rendtime = time.time()
    print(f"Total of {len(filelist)} files found. ({(rendtime-rstarttime):.2f} seconds)")
    astarttime = time.time()
    tc = 0.0
    for x in filelist:
        starttime = time.time()
        with open(x[0], 'rb') as srcfile:
            filedata = srcfile.read()
            totalbytes += len(filedata)
            totalpackets += int(len(filedata) / MAX_PAYLOAD) + 1
            #print(f"{x[0]} {len(filedata)}")
            cstarttime = time.time()

            compressed = compress(filedata, x[0])
            #print(f"{x[0]} {len(filedata)} -> {len(compressed)}")

            endtime = time.time()
            """
            if len(filedata) < len(compressed):
                print(f"{x[0]} {len(filedata)} -> {len(compressed)}")
                compressedbytes += len(filedata)
                compressedpackets += int(len(filedata) / MAX_PAYLOAD) + 1
            else:
                compressedbytes += len(compressed)
                compressedpackets += int(len(compressed) / MAX_PAYLOAD) + 1
            """
            compressedbytes += len(compressed)
            compressedpackets += int(len(compressed) / MAX_PAYLOAD) + 1
            tc += endtime - starttime
            
            dstart = time.time()
            decompressed = decompress(compressed)
            dend = time.time()
            print(f"decompress {dend-dstart}")
            if len(decompressed) != len(filedata):
                print(f"decompress size mismatch {len(decompressed)} != {len(filedata)}")
            else:
                if filedata != decompressed:
                    print("data mismatch")
                else:
                    print("check ok")
            
            #print(f"{len(filedata)} -> {len(compressed)} {(len(compressed)/len(filedata)):.2f} {(len(filedata)/1024)/(endtime-cstarttime):.2f}k/s load:{(cstarttime-starttime):.2f} comp:{(endtime-cstarttime):.2f} total:{(endtime-starttime):.2f} ")
            #with open("packed.dat", "wb") as dstfile:
            #    dstfile.write(compressed)
    aendtime = time.time()
    print(f"Total bytes       :{totalbytes} {totalbytes//(1024*1024)}M\nCompressed bytes  :{compressedbytes}\nTotal packets     :{totalpackets}\nCompressed packets:{compressedpackets}")
    print(f"({(aendtime-astarttime):.2f} {tc:.2f})") 
    
main()
