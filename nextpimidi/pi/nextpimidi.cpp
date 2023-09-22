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
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h> // sched_yield
#include "nextgpio.h"

enum RUNSTATE
{
    RUN_IDLE,
    RUN_RECEIVE,
    RUN_STARTSEND,
    RUN_SEND
};

volatile RUNSTATE gRunstate = RUN_IDLE;

// Data queue, filled from MIDI events
std::queue<unsigned char> dq;


int gAppControlPatterns[4 * 2] = 
{
    0,15, // Quit
    10,12, // move to receive mode
    10,3, // move to send mode
    10,6 // move to idle mode
};


// rtmidi callback, we'll just put the MIDI messages in a queue to be sent down to next 
void mycallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    unsigned int nBytes = message->size();
    if (nBytes >= 3)
    {
        if (gRunstate == RUN_RECEIVE)
        {
            dq.push(message->at(0));
            dq.push(message->at(1));
            dq.push(message->at(2));
        }
    }
}

void init_receive()
{
    gRunstate = RUN_IDLE;
    // clear data queue
    while (!dq.empty()) dq.pop();
    // Send "MID" first, so the client knows we're good
    dq.push('M');
    dq.push('I');
    dq.push('D');

    // write 8 bits starting at 16
    for (int i = 0; i < 8; i++)    
        nextgpio_config_io(i + 16, 1);
    nextgpio_set_byte(16, 0);    
    gRunstate = RUN_RECEIVE;
}

void init_idle()
{
    gRunstate = RUN_IDLE;
    // clear data queue
    while (!dq.empty()) dq.pop();

    // Restore the data pins to original state
    for (int i = 0; i < 8; i++)    
        nextgpio_restore_io(i + 16);
}

void init_send()
{
    gRunstate = RUN_IDLE;
    // clear data queue
    while (!dq.empty()) dq.pop();

    // read 8 bits starting at 16
    for (int i = 0; i < 8; i++)    
        nextgpio_config_io(i + 16, 0);

    gRunstate = RUN_STARTSEND;
}

int main()
{

    int ticktock = 0;
    if (nextgpio_init())
    {
        return 0;
    }
    atexit(nextgpio_deinit);

    // write pin 4
    nextgpio_config_io(4, 1);
    // read pin 5 
    nextgpio_config_io(5, 0);
    // clear the pins
    nextgpio_set_val(4, 0);

    RtMidiIn *midiin = new RtMidiIn();
    RtMidiOut *midiout = new RtMidiOut();
 
    // Check available input ports.
    unsigned int inPorts = midiin->getPortCount();
    printf("%d in ports\n", inPorts);
    if ( inPorts == 0 ) 
    {
        std::cout << "No input ports available!\n";
        delete midiin;
        delete midiout;
        return 1;
    }
    midiin->openPort(inPorts-1); 
 
    // Check available output ports.
    unsigned int outPorts = midiout->getPortCount();
    printf("%d out ports\n", outPorts);
    if ( outPorts == 0 ) 
    {
        std::cout << "No output ports available!\n";
        delete midiin;
        delete midiout;
        return 1;
    }
    midiout->openPort(outPorts-1); 

    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the
    // queue.
    midiin->setCallback( &mycallback );
 
    // Don't ignore sysex, timing, or active sensing messages.
    midiin->ignoreTypes( false, false, false );
 
    printf("Running..\n");
    while(1)
    {
        int appctl = nextgpio_app_control(gAppControlPatterns,4);
        switch (appctl)
        {
            case 1:
                delete midiin;
                delete midiout;
                return 0;
                break;
            case 2: 
                init_receive();
                break;
            case 3:
                init_send();
                break;
            case 4:
                init_idle();
                break;
        }

        switch (gRunstate)
        {
        case RUN_RECEIVE:
            if (!dq.empty())
            {
                if (nextgpio_get_val(5) != ticktock)
                {
                    unsigned char d = dq.front();
                    dq.pop();
                    nextgpio_set_byte(16, d);
                    ticktock = !ticktock;
                    nextgpio_set_val(4, ticktock); // toggle our ready bit        
                }
            }
            break;
        case RUN_SEND:
            if (nextgpio_get_val(5) != ticktock)
            {                    
                unsigned char d = nextgpio_get_byte(16);
                dq.push(d);
                ticktock = !ticktock;
                nextgpio_set_val(4, ticktock); // toggle our ready bit        
            }
            if (dq.size() >= 3)
            {
                std::vector<unsigned char> message;
                message.push_back(dq.front()); dq.pop();
                message.push_back(dq.front()); dq.pop();
                message.push_back(dq.front()); dq.pop();
                midiout->sendMessage(&message);
            }
            break;
        case RUN_STARTSEND:
            {
                // Expect MID, accept ID
                if (nextgpio_get_val(5) != ticktock)
                {                    
                    unsigned char d = nextgpio_get_byte(16);
                    if (d == 'M' || d == 'I' || d == 'D')
                        dq.push(d);
                    ticktock = !ticktock;
                    nextgpio_set_val(4, ticktock); // toggle our ready bit        
                }

                if (dq.size() == 2 && dq.front() == 'I')
                {
                    dq.pop();
                    if (dq.front() == 'D') gRunstate = RUN_SEND;
                    
                    dq.pop();
                }

                if (dq.size() == 3 && dq.front() == 'M')
                {
                    int ok = 1;
                    dq.pop();
                    if (dq.front() != 'I') ok = 0;
                    dq.pop();
                    if (dq.front() != 'D') ok = 0;
                    dq.pop();
                    if (ok) gRunstate = RUN_SEND;
                }
                else
                if (dq.size() >= 3) dq.pop();
            }
            break;
        }
        
        sched_yield(); 
    }
 
    delete midiin;
    delete midiout;

    return 0;
}