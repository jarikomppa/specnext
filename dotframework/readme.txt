This is a simple sdcc-based framework to make dot executables.

Some caveats:
- Resulting binary must be under 8k.
- Stack is set to the end of that 8k, so probably better to be smaller than 8k.
- First two banks cannot be remapped with mmu. The dot code resides at the second bank.
- No static variables, no non-const globals. If globals are needed, declare them as extern and add them to a .s file.
  See framecounter as example.
- If you muck with mmu, make sure you restore the pages before exiting.
- You're more likely to be better off using z88dk than this framework, but ymmv.

Features:
- Custom text output (drawstringz)
- Disk i/o (fopen,fclose,fread,fwrite)
- Memory allocation (specnext banks)
- Memory mapping (nextreg read/write)
- ay register writes
- interrupt service hook

