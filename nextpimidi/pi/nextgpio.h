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

#ifndef NEXTGPIO_H_INCLUDED
#define NEXTGPIO_H_INCLUDED

// Init. Call this first. May fail. 0 means success.
int nextgpio_init();

// Deinit. Call this last. Can be atexit().
void nextgpio_deinit();

// Configure pin as read (0) or write (1)
void nextgpio_config_io(int pin, int output);

// Set pin value (to 1 or 0)
void nextgpio_set_val(int pin, int val);

// Set 8 continious pins values starting from specific pin.
void nextgpio_set_byte(int pinofs, int val);

// Get pin value (0 or 1)
int nextgpio_get_val(int pin);

// Get 8 continious pins values starting from specific pin
int nextgpio_get_byte(int pinofs);

// Check whether your service application should quit. Do this often.
int nextgpio_should_quit();

#endif