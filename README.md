# libPBX

A Raspberry Pi driver library for the [Pixelblaze 8-64 channel expander](https://www.bhencke.com/serial-led-driver), based on Ben Hencke's [PBDriverAdapter library](https://github.com/simap/PBDriverAdapter) for Arduino.

It depends on [wiringPi](wiringpi.com). A note for users of newer Raspberry Pis: the original creator/maintainer of wiringPi dropped the project, so if you need to get the updated version for 4b, try http://wiringpi.com/wiringpi-updated-to-2-52-for-the-raspberry-pi-4b/ or https://github.com/WiringPi/WiringPi (which seems to still be somewhat active?).

Currently supports WS2812 family LEDs.

# Usage example: 'bounce'
```c
#include <wiringPi.h> /* for delay() */
#include <math.h>
#include <errno.h>

#include "pbx.h"

#define NUMPIXELS 10

int main ()
{
    lx_pbx_init (1);
    lx_pbx_driver_t pbx;
    lx_pbx_driver_create ("/dev/ttyS0", &pbx)
    lx_pbx_ws2812_chan_t chan;
    lx_pbx_open_channel_ws2812 (
        /* channel number= */0,
        &chan,
        LX_PBX_CHANNEL_RGB,
        /* Send the green component first, followed by red and then blue; the white
        component is ignored */
        1, 0, 2, 0);
    int bounce_pos  = 0;
    int bounce_step = 1;
    while (1) {
        uint8_t data[NUMPIXELS][3];
        uint8_t r, g, b;
        for (int i = 0; i < NUMPIXELS; i++) {
            if (bounce_pos == i) {
                data[i][0] = 0x85;
                data[i][1] = 0x24;
                data[i][2] = 0x21;
            } else {
                data[i][0] = 0x48;
                data[i][1] = 0x16;
                data[i][2] = 0x84;
            }
        }
        bounce_pos += bounce_step;
        if (bounce_pos == NUMPIXELS) bounce_step = -1, bounce_pos = NUMPIXELS - 2;
        if (bounce_pos == -1) bounce_step = +1, bounce_pos = 1;

        lx_pbx_driver_write_ws2812_chan (&pbx, &chan, (uint8_t *)&data, NUMPIXELS);
        lx_pbx_driver_draw_accumulated (&pbx);

        delay (10);
    }

    lx_pbx_driver_destroy (&pbx);
    return 0;
}
```

# Building

```
git clone https://github.com/bosporos/libpbx.git
cd libpbx
clang -c -o libpbx.so pbx.c
```

# API

### `int lx_pbx_init(int needs_wpi_setup)`

Should be called once at the beginning of the program.

If `needs_wpi_setup` is nonzero, then it `lx_pbx_init` will call `wiringPiSetup()`. If you are separately initializing wiringPi (e.g. calling one of the `wiringPiSetupXXX()` functions), then make sure to pass `0` so that wiringPi does not get initialized twice.

Returns 0 on success.

### `int lx_pbx_driver_create(const char * device_file, lx_pbx_driver_t * driver)`

Initialize a new driver attached to a given serial port. `device_file` should be the path to the appropriate serial port (e.g. `/dev/ttyS0`, etc.).

Example:
```
lx_pbx_driver_t my_driver;
lx_pbx_driver_create("/dev/serial0", &my_driver);
```

Returns 0 on success, `LX_PBX_NO_DRIVER` if an error occurs. In the event of an error, the error will be in `errno`.

### `int lx_pbx_driver_write_ws2812_chan(const lx_pbx_driver_t * driver, lx_pbx_ws2812_chan_t channel, uint8_t * pixel_data, size_t pixel_num)`

Buffer `pixel_data` to a given channel according to the settings in `channel`.
To actually draw the buffered pixel data, use `lx_pbx_driver_draw_accumulated(lx_pbx_driver_t * driver)`.

`pixel_data` should be a contiguous block of memory containing the data of `pixel_num` pixels, formatted according to `channel->channel_type`. `LX_PBX_CHANNEL_RGB` specifies that each set of three bytes will be one byte of red, one byte of blue, one byte of green, in that order.

By setting `pixel_data` to NULL, `pixel_num` to 0, and `channel->channel_type` to `LX_PBX_CHANNEL_DISABLED`, then channel may be disabled.

Returns 0 on success, otherwise will return the error. Possible errors: EINVAL, ENXIO.

### `int lx_pbx_driver_draw_accumulated(lx_pbx_driver_t * driver)`

Draws buffered channel from all channels.

Returns 0 on success, otherwise will return the value of the error. Possible errors: EINVAL, ENXIO.

### `int lx_pbx_driver_destroy(lx_pbx_driver_t * driver)`

Teardown function, closes the serial port that `driver` has open.

Will return 0 on success, otherwise the value of the error. Possible errors: EINVAL, ENXIO.

### `int lx_pbx_open_channel_ws2812 (uint8_t chan_no, lx_pbx_ws2812_chan_t * channel, uint8_t chan_type, uint8_t red_i, uint8_t green_i, uint8_t blue_i, uint8_t white_i)`

Initializes `channel`, binding it to channel number `chan_no` and changing its type to `chan_type`. `red_i`, `green_i`, `blue_i`, and `white_i` are used to specify the order in which color components are sent to the LEDs. (You may need to tinker around with this to get the colors to come out properly)

Returns 0 on success, otherwise the value of the error. Will return EINVAL if `channel` is invalid, `ENOTSUP` if `chan_type` is not recognized, EDOM if any of `red_i`, etc. are greater than 3.

### `int lx_pbx_set_channel_comp_ws2812 (lx_pbx_ws2812_chan_t * channel, uint8_t chan_type)`

Changes the channel type of `channel` to `chan_type`.

Returns 0 on success, otherwise the value of the error. Will return EINVAL if `channel` is invalid, and `ENOTSUP` if `chan_type` is not recognized.

# Troubleshooting

- It may be necessary to fiddle around with the color component ordering in `lx_pbx_open_channel_ws2812`. (For the strip of ws2812b's I was testing with, I ended up with 1,0,2 for red, green, blue, but I'm not sure if that'll work for everyone).

- On Raspberry Pi (I use Ubuntu Server as my OS), I had to disable the default-enabled serial console by removing `console=/dev/<serial port>,115200` from `/boot/cmdline.txt`.

- You may need a logic level converter between your Pi and the Pixelblaze Expander.
