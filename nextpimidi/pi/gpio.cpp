/*
specnext regs 0x90-0x93 - 32 bits
dead 0-1 (internal to pi0)
I2C 2-3 (-> RTC)
SPI 7-11 (-> SD)
UART 14-15
Pins 28-31 do not exist in pi0 hardware

leaves..

0x90------------------ 0x91------------------- 0x92------------------- 0x93------------------- (output enable)
0x98------------------ 0x99------------------- 0x9A------------------- 0x9B------------------- (i/o)
0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
-dead-I2C-          -SPI----------       -UART                                    -no hardware
*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> // mmap
#include <fcntl.h> // open
#include <unistd.h> // close (?!)
#include <sched.h> // sched_yield

volatile static unsigned int *gpio_mmap = 0; 

int init_gpio() 
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
  
  return 0;
}

void gpio_deinit()
{
    if (gpio_mmap)
    {
        munmap((void*)gpio_mmap, 4*1024);
    }
    gpio_mmap = 0;
}

void set_gpio_io(int pin, int output)
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

void set_gpio_val(int pin, int val)
{
    if (!gpio_mmap) return;
    int block = (val ? 7 : 10); // SET and CLEAR register offsets
    __sync_synchronize(); 
    *(gpio_mmap + block) = 1 << pin;
    __sync_synchronize(); 
}

int get_gpio_val(int pin)
{
    if (!gpio_mmap) return 0;
    int block = 13; // LEVEL register offset
    __sync_synchronize(); 
    int v = !!(*(gpio_mmap + block) & (1 << pin)); 
    __sync_synchronize(); 
    return v;
}


int main(int parc, char **pars)
{
    if (init_gpio()) 
    {
        return 0;
    }
    atexit(gpio_deinit);

    // write 8 bits starting at 16, + 1 bit starting 24
    for (int i = 16; i < 25; i++)    
        set_gpio_io(i, 1);
    // read pin 25
    set_gpio_io(25, 0);
    
    // clear the pins
    for (int i = 16; i < 25; i++)    
        set_gpio_val(i, 0);
    
    int ticktock = 0;
    printf("Running..\n");
    while(1)
    {
        // wait for next to toggle bit
        while (get_gpio_val(25) == ticktock) { sched_yield();}
        for (int i = 16; i < 24; i++)    
            set_gpio_val(i, rand() & 1); // set 8 bits to random values
        ticktock = !ticktock;
        set_gpio_val(24, ticktock); // toggle our ready bit        
    }
    
    return 0;
}