/**
 ** Very simple nextpi gpio library
 ** by Jari Komppa 2023, http://iki.fi/sol
 **/
 
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute
this software, either in source code form or as a compiled binary, for any
purpose, commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of
this software dedicate any and all copyright interest in the software to
the public domain. We make this dedication for the benefit of the public
at large and to the detriment of our heirs and successors. We intend this
dedication to be an overt act of relinquishment in perpetuity of all
present and future rights to this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org/>
*/

/*
Nextpi GPIO
===========
- specnext regs 0x90-0x93 - 32 bits
- Pins 0-1 are dead (internal to pi0)
- Pins 2-3 I2C (-> RTC, probably doesn't work?)
- Pins 7-11 SPI (-> SD, very likely doesn't work)
- Pins 14-15 UART (-> term, pisend, etc)
- Pins 18-21 I2S (-> audio, works and is used)
- Pins 28-31 do not exist in pi0 hardware

- Pins 24-27 reserved for app control

Map:

0x90------------------ 0x91------------------- 0x92------------------- 0x93------------------- (output enable)
0x98------------------ 0x99------------------- 0x9A------------------- 0x9B------------------- (i/o)
0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
XX XX XX XX          XX XX XX XX XX       XX XX       XX XX XX XX                   XX XX XX XX
-dead-I2C-          -SPI-----------       -UART       -I2S-------       -AppCtl---- -no hardware

Plan:

            AA BB CC                            DD DD DD DD DD DD DD DD EE FF GG HH
            
A = pi-side 1 bit data counter
B = next-side 1 bit data counter
C = transfer direction, 0 = pi->next, 1 = next->pi. Controlled by next.
D = 8 bits of data (overlaps with I2S, so no sound while this is going)
E,F,G,H = App control. If a pattern of 0 0 0 0 ->  1 1 1 1 is seen, 
          service should clean up after itself and die.

*/

//#define INCLUDE_TESTCODE

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> // mmap
#include <fcntl.h> // open
#include <unistd.h> // close (?!)
#include <sched.h> // sched_yield
#include "nextgpio.h"

volatile static unsigned int *gpio_mmap = 0; 

static unsigned int gpio_original_io[3];
static unsigned int gpio_original_val;
static unsigned char gpio_prevappctl = 0x37;

int nextgpio_should_quit()
{
    if (!gpio_mmap) return 1;
    int block = 13; // LEVEL register offset
    __sync_synchronize(); 
    int v = *(gpio_mmap + block); 
    __sync_synchronize(); 
    unsigned char currstate = (v >> 24) & 15;
    int ret = (gpio_prevappctl == 0 && currstate == 15);
    gpio_prevappctl = currstate;
    return ret;   
}

int nextgpio_app_control(int *pair, int pairs)
{
    if (!gpio_mmap) return 1;
    int block = 13; // LEVEL register offset
    __sync_synchronize(); 
    int v = *(gpio_mmap + block); 
    __sync_synchronize(); 
    unsigned char currstate = (v >> 24) & 15;
    int ret = 0;
    for (int i = 0; i < pairs; i++)
        if (gpio_prevappctl == pair[i*2+0] && currstate == pair[i*2+1])
            ret = i + 1;
    gpio_prevappctl = currstate;
    return ret;   
}

int nextgpio_init() 
{ 
    int gpio_file = -1;
  
    gpio_file = open("/dev/gpiomem", O_RDWR | O_SYNC);
    if (gpio_file < 0) 
    {
        printf("gpio open failed\n");
        return 1;
    }
  
    gpio_mmap = (unsigned int*)mmap(
        NULL,					// Address offset, docs say to use null =)
        4*1024,				    // Map Length
        PROT_READ|PROT_WRITE,	// Enable Read and Write
        MAP_SHARED,				// The map can be used by other processes
        gpio_file, 			    // The file opened for the map
        0						// Offset
    );

    // mmap done, don't need file anymore
    close(gpio_file);
  
    if (gpio_mmap == MAP_FAILED) 
    {
        printf("gpio mmap failed\n");
        gpio_mmap = 0;
        return 1;
    }
    
    __sync_synchronize(); 
    gpio_original_io[0] = *(gpio_mmap + 0);
    __sync_synchronize(); 
    gpio_original_io[1] = *(gpio_mmap + 1);
    __sync_synchronize(); 
    gpio_original_io[2] = *(gpio_mmap + 2);
    __sync_synchronize(); 
    gpio_original_val = *(gpio_mmap + 13);
    __sync_synchronize(); 
 
    // Set pins 4-7 as read, as we're reading them for quit signal
    nextgpio_config_io(24, 0);
    nextgpio_config_io(25, 0);
    nextgpio_config_io(26, 0);
    nextgpio_config_io(27, 0);
  
  return 0;
}

void nextgpio_deinit()
{
    // return to original io state
    __sync_synchronize(); 
    *(gpio_mmap + 0) = gpio_original_io[0];
    __sync_synchronize(); 
    *(gpio_mmap + 1) = gpio_original_io[1];
    __sync_synchronize(); 
    *(gpio_mmap + 2) = gpio_original_io[2];
    __sync_synchronize(); 
    
    
    if (gpio_mmap)
    {
        munmap((void*)gpio_mmap, 4*1024);
    }
    gpio_mmap = 0;
}

void nextgpio_config_io(int pin, int output)
{
    if (!gpio_mmap) return;
    // input or output state is set as 3 bits per pin, stored as 10 pins per dword
    // 000 = input, 001 = output, (010 = special peripheral whatever)
    int block = pin / 10;
    int pinofs = (pin % 10) * 3;
    // mask out the 3 bits and set the one bit as requested
    __sync_synchronize(); 
    unsigned int d = *(gpio_mmap + block);
    __sync_synchronize(); 
    *(gpio_mmap + block) = (d & ~(7 << pinofs)) | (output << pinofs);
    __sync_synchronize(); 
}

void nextgpio_restore_io(int pin)
{
    if (!gpio_mmap) return;
    // input or output state is set as 3 bits per pin, stored as 10 pins per dword
    // 000 = input, 001 = output, (010 = special peripheral whatever)
    int block = pin / 10;
    int pinofs = (pin % 10) * 3;
    // mask out the 3 bits and set the one bit as requested
    __sync_synchronize(); 
    unsigned int d = *(gpio_mmap + block);
    __sync_synchronize(); 
    *(gpio_mmap + block) = (d & ~(7 << pinofs)) | (gpio_original_io[block] & (7 << pinofs));
    __sync_synchronize(); 
}

void nextgpio_set_val(int pin, int val)
{
    if (!gpio_mmap) return;
    int block = (val ? 7 : 10); // SET and CLEAR register offsets
    __sync_synchronize(); 
    *(gpio_mmap + block) = 1 << pin;
    __sync_synchronize(); 
}

void nextgpio_set_byte(int pinofs, int val)
{
    for (int i = 0; i < 8; i++)    
        nextgpio_set_val(i + pinofs, !!(val & (1 << i))); 
}

int nextgpio_get_val(int pin)
{
    if (!gpio_mmap) return 0;
    int block = 13; // LEVEL register offset
    __sync_synchronize(); 
    int v = !!(*(gpio_mmap + block) & (1 << pin)); 
    __sync_synchronize(); 
    return v;
}

int nextgpio_get_byte(int pinofs)
{
    if (!gpio_mmap) return 0;
    int block = 13; // LEVEL register offset
    __sync_synchronize(); 
    int v = *(gpio_mmap + block); 
    __sync_synchronize(); 
    return (v >> pinofs) & 0xff;
}

#if INCLUDE_TESTCODE
int main(int parc, char **pars)
{
    if (nextgpio_init()) 
    {
        return 0;
    }
    atexit(nextgpio_deinit);

    // write 8 bits starting at 16
    for (int i = 0; i < 8; i++)    
        nextgpio_config_io(i + 16, 1);
    // write pin 4
    nextgpio_config_io(4, 1);
    // read pin 5 and 6
    nextgpio_config_io(5, 0);
    nextgpio_config_io(6, 0);

    // clear the pins
    nextgpio_set_byte(16, 0);    
    nextgpio_set_val(4, 0);
    
    int ticktock = 0;
    printf("Running..\n");
    while(1)
    {
        // wait for next to toggle bit
        while (nextgpio_get_val(5) == ticktock) 
        { 
            if (nextgpio_should_quit())
                return 0;
            sched_yield(); 
        }
        nextgpio_set_byte(16, rand()); // set 8 bits to random values
        ticktock = !ticktock;
        nextgpio_set_val(4, ticktock); // toggle our ready bit        
    }
    
    return 0;
}
#endif