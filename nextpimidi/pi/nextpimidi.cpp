// g++ RtMidi.cpp nextpimidi.cpp -Os -lpthread -D__LINUX_ALSA__ -lasound -static-libstdc++ -s
 
#include <iostream>
#include <cstdlib>
#include "RtMidi.h"
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> // mmap
#include <fcntl.h> // open
#include <unistd.h> // close (?!)
#include <sched.h> // sched_yield

static unsigned int *gpio_mmap = 0; 

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

std::queue<unsigned char> dq;

 
void mycallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    unsigned int nBytes = message->size();
    if (nBytes >= 3)
    {
        dq.push(message->at(0));
        dq.push(message->at(1));
        dq.push(message->at(2));
    }
}
 
int main()
{
    int ticktock = 0;
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

 RtMidiIn *midiin = new RtMidiIn();
 
  // Check available ports.
  unsigned int nPorts = midiin->getPortCount();
  printf("%d ports\n");
  if ( nPorts == 0 ) {
    std::cout << "No ports available!\n";
    goto cleanup;
  }
 
  midiin->openPort( nPorts-1 );
 
  // Set our callback function.  This should be done immediately after
  // opening the port to avoid having incoming messages written to the
  // queue.
  midiin->setCallback( &mycallback );
 
  // Don't ignore sysex, timing, or active sensing messages.
  midiin->ignoreTypes( false, false, false );
 
    printf("Running..\n");
    while(1)
    {
        // wait for data in queue
        while (dq.empty()) { sched_yield(); }
        // wait for next to toggle bit
        while (get_gpio_val(25) == ticktock) { sched_yield(); }
        unsigned char d = dq.front();
        dq.pop();
        for (int i = 0; i < 8; i++)    
            set_gpio_val(i + 16, !!(d & (1 << i))); // set 8 bits to data values
        ticktock = !ticktock;
        set_gpio_val(24, ticktock); // toggle our ready bit        
    }
 
  // Clean up
 cleanup:
  delete midiin;
 
  return 0;
}