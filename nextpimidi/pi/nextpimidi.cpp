// g++ nextgpio.cpp RtMidi.cpp nextpimidi.cpp -Os -lpthread -D__LINUX_ALSA__ -lasound -static-libstdc++ -s
 
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
#include "nextgpio.h"


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
     if (nextgpio_init() 
    {
        return 0;
    }
    atexit(nextgpio_deinit);

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
    // that seems to always be 2.
    printf("%d ports\n");
    if ( nPorts == 0 ) 
    {
        std::cout << "No ports available!\n";
        goto cleanup;
    }
 
  midiin->openPort(nPorts-1); // last port seems to work

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
        while (dq.empty()) 
        { 
            if (nextgpio_should_quit())
                goto cleanup;
            sched_yield(); 
        }
        // wait for next to toggle bit
        while (nextgpio_get_val(25) == ticktock) 
        { 
            if (nextgpio_should_quit())
                goto cleanup;
            sched_yield(); 
        }
        unsigned char d = dq.front();
        dq.pop();
        nextgpio_set_byte(16, d);
        ticktock = !ticktock;
        nextgpio_set_val(24, ticktock); // toggle our ready bit        
    }
 
  // Clean up
cleanup:
  delete midiin;
 
  return 0;
}