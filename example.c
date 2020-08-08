//
// file example.c
// author Maximilien M. Cura
//

#include <stdio.h>
#include <termios.h>
#include <math.h>
#include "pbx.h"
#include <errno.h>
#include <unistd.h>
#include <wiringPi.h>

const char * dev = "/dev/ttyS0";
#define NUMPIXELS 240

#define SINEWAVE
//#define BOUNCE

int main ()
{
    lx_pbx_init (1);
    lx_pbx_driver_t pbx;
    if (LX_PBX_NO_DRIVER == lx_pbx_driver_create (dev, &pbx)) {
        printf ("Could not open [%s]! errno: %i", dev, errno);
        return 1;
    }

    lx_pbx_ws2812_chan_t chan;
    lx_pbx_open_channel_ws2812 (0, &chan, LX_PBX_CHANNEL_RGB, 1, 0, 2, 0);

#ifdef SINEWAVE
    float sine_pos = 0;
#elif defined(BOUNCE)
    int bounce_pos  = 0;
    int bounce_step = 1;
#endif
#if defined(SINEWAVE) || defined(BOUNCE)
    while (1) {
#endif
        uint8_t data[NUMPIXELS][3];
        uint8_t r, g, b;
        for (int i = 0; i < NUMPIXELS; i++) {
#ifdef SINEWAVE
            r = (float)0x148 * pow (sin (sine_pos + 0.1 * i) / 2.0 + 0.5, 2);
            g = (float)0x43 * pow (sin (sine_pos + 0.1 * i) / 2.0 + 0.5, 2);
            b = (float)0x197 * pow (sin (sine_pos + 0.1 * i) / 2.0 + 0.5, 2);
#elif defined(BOUNCE)
            if (bounce_pos == i) {
                r = 0x85;
                g = 0x24;
                b = 0x21;
            } else {
                r = 0x48;
                g = 0x16;
                b = 0x84;
            }
#else /* SOLID */
            r = 0x00;
            g = 0x00;
            b = 0x00;
#endif
            data[i][0] = (uint8_t)r;
            data[i][1] = (uint8_t)g;
            data[i][2] = (uint8_t)b;
        }
#ifdef SINEWAVE
        sine_pos += 0.02;
#elif defined(BOUNCE)
        bounce_pos += bounce_step;
        if (bounce_pos == NUMPIXELS)
            bounce_step = -1, bounce_pos = NUMPIXELS - 2;
        if (bounce_pos == -1)
            bounce_step = +1, bounce_pos = 1;
#endif
        lx_pbx_driver_write_ws2812_chan (&pbx, &chan, (uint8_t *)&data, NUMPIXELS);
        // draw
        lx_pbx_driver_draw_accumulated (&pbx);
        delay (10);
#if defined(SINEWAVE) || defined(BOUNCE)
    }
#endif

    lx_pbx_driver_destroy (&pbx);
    return 0;
}
