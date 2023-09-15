/**
 ** Nextpi USB MIDI service
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
// To compile:
// g++ nextgpio.cpp RtMidi.cpp nextpimidi.cpp -Os -lpthread -D__LINUX_ALSA__ -lasound -s
 
#include <iostream>
#include <cstdlib>
#include "RtMidi.h"
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h> // sched_yield
#include "nextgpio.h"

// Data queue, filled from MIDI events
std::queue<unsigned char> dq;

// rtmidi callback, we'll just put the MIDI messages in a queue to be sent down to next 
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
    // Send "MID" first, so the client knows we're good
    dq.push('M');
    dq.push('I');
    dq.push('D');

    int ticktock = 0;
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
        while (nextgpio_get_val(5) == ticktock) 
        { 
            if (nextgpio_should_quit())
                goto cleanup;
            sched_yield(); 
        }
        unsigned char d = dq.front();
        dq.pop();
        nextgpio_set_byte(16, d);
        ticktock = !ticktock;
        nextgpio_set_val(4, ticktock); // toggle our ready bit        
    }
 
  // Clean up
cleanup:
    delete midiin;

    return 0;
}