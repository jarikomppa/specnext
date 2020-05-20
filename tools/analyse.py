#!/usr/bin/env python3

# Part of Jari Komppa's zx spectrum next suite 
# https://github.com/jarikomppa/specnext
# released under the unlicense, see http://unlicense.org 
# (practically public domain) 

import glob

items = []
strings = []
strsize = 0
totalsize = 0

with open("crt0.ihx") as f:
    lines = f.read().splitlines()
    start = int(lines[0][3:7],16)
    end = int(lines[-2][3:7],16)
    last = int(lines[-2][1:3],16)
    totalsize = end - start + last + 1
    print("crt0.ihx analysis:")
    print(f"Binary size {totalsize}, offset {start}")
print()

for fn in glob.glob("*.lst"):
    with open(fn) as f:
        lines = f.read().splitlines()
        lastofs = 0
        lastsym = ""
        prevofs = 0
        timetravel = 0
        ofs = 0
        for x in lines:
            if len(x) > 32:
                if x[0:3] == "   " and x[3] != ' ':                    
                    ofs = int(x[3:7], 16)
                    if ofs < prevofs:
                        timetravel = prevofs - lastofs + 1 # guess, but better than nothing
                    prevofs = ofs
                    if ':' in x and x[32] == '_':
                        if lastsym != "":
                            delta = ofs - lastofs
                            if delta < 0:
                                delta = timetravel
                            if "___str" in lastsym:
                                strings.append([delta, lastsym])
                                strsize += delta
                            else:
                                items.append([delta, lastsym])
                        lastofs = ofs
                        lastsym = x[32:x.rfind(':')]
        # end of file, store last
        if "_endof" not in lastsym:
            delta = ofs - lastofs
            if delta < 0:
                delta = timetravel
            if "___str" in lastsym:
                strings.append([delta, lastsym])
                strsize += delta
            else:
                items.append([delta, lastsym])
    
items.append([strsize, "Literal strings"])

print("*.lst analysis:\n- Last symbols per file may be under-estimated\n- Adding _endofxxxx: label to end of .s files may help")

total = 0

for x in sorted(items, reverse=True):
    print(f"{x[0]:{5}} {x[1]}")
    total += x[0]

print(f"Total {total} bytes ({totalsize - total} bytes unaccounted for)\n\nLongest strings:")

for x in sorted(strings, reverse=True)[0:10]:
    print(f"{x[0]:{5}} {x[1]}")
    total += x[0]

print()               

with open("crt0.noi") as f:
    lines = f.read().splitlines()
    nois = []
    headerofs = 0
    codeofs = 0
    finalofs = 0
    for x in lines:
        if x[4] == '_' and "_heap" not in x:
            nois.append([int(x[-4:],16),x[4:-7]])
        if "s__HEADER" in x:
            headerofs = int(x[-4:],16)
        if "s__CODE" in x:
            codeofs = int(x[-4:],16)
        if "s__GSFINAL" in x:
            finalofs = int(x[-4:],16)
    print("crt0.noi analysis:")
    print(f"Header offset {headerofs}\nHeader size {codeofs-headerofs}\nCode offset {codeofs}\nBinary size {finalofs-headerofs + 1}")
    
    lastofs = codeofs
    lastlabel = ""
    items = []
    outside = 0    
    for x in sorted(nois):
        if x[0] >= codeofs and x[0] <= finalofs:
            delta = x[0] - lastofs
            if lastlabel != "":
                items.append([delta, lastlabel])
            lastofs = x[0]
            lastlabel = x[1]
        else:
            if outside == 0:
                print("Items outside code segment:")
                outside = 1
            print(f"{x[0]-headerofs:{5}} {x[1]}")
    delta = finalofs - lastofs
    print("Item sizes:")
    items.append([delta, lastlabel])
    for x in sorted(items, reverse=True):
        print(f"{x[0]:{5}} {x[1]}")
