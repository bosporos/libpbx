//
// file pbx.c
// author Maximilien M. Cura
//

#include "pbx.h"

#include <wiringPi.h> /* wiringPiSetup() */
#include <wiringSerial.h> /* serial library */

#include <unistd.h> /* write(fd,dbuf,sz) */
#include <errno.h>
#include <sched.h>

uint32_t __crc_update (uint32_t crc, const void * new_data, size_t dlen);
void __crc_init (uint32_t * crc);
void __crc_send (uint32_t * crc, const lx_pbx_driver_t * driver);

void __lxpbx_write_record_header (lx_pbx_rec_hdr_t * rh,
                                  uint32_t * crc,
                                  const lx_pbx_driver_t * driver);
void __lxpbx_write_ws2812_chaninfo (lx_pbx_ws2812_chan_t * chaninf,
                                    uint32_t * crc,
                                    const lx_pbx_driver_t * driver);
void __lxpbx_write (const void * data,
                    size_t dlen,
                    uint32_t * crc,
                    const lx_pbx_driver_t * driver);

int lx_pbx_init (int needs_wpi_setup)
{
    if (needs_wpi_setup) {
        wiringPiSetup ();
    }
    return 0;
}

int lx_pbx_driver_create (const char * device_file,
                          lx_pbx_driver_t * driver)
{
    int fd = serialOpen (device_file, LX_PBX_BAUD_RATE);
    if (fd == -1) {
        /* error in errno */
        return LX_PBX_NO_DRIVER;
    }
    driver->fd        = fd;
    driver->last_draw = micros ();
    return 0;
}

int lx_pbx_driver_write_ws2812_chan (const lx_pbx_driver_t * driver,
                                     lx_pbx_ws2812_chan_t * channel,
                                     uint8_t * pixel_data,
                                     size_t pixel_num)
{
    if (driver == NULL)
        return EINVAL;
    if (driver->fd < 0)
        return ENXIO;

    uint32_t crc;
    __crc_init (&crc);
    lx_pbx_rec_hdr_t record_header = {
        .magic_bytes = { 'U', 'P', 'X', 'L' },
        .channel_no  = channel->channel_no,
        .record_type = LX_PBX_RECORD_CHANNEL_WS2812
    };
    __lxpbx_write_record_header (&record_header, &crc, driver);
    channel->pixels = pixel_num;
    __lxpbx_write_ws2812_chaninfo (channel, &crc, driver);
    for (size_t i = 0; i < pixel_num; i++) {
        __lxpbx_write (pixel_data, channel->channel_width, &crc, driver);
        pixel_data += channel->channel_width;
    }
    __crc_send (&crc, driver);

    return 0;
}

int lx_pbx_driver_draw_accumulated (lx_pbx_driver_t * driver)
{
    if (driver == NULL)
        return EINVAL;
    if (driver->fd < 0)
        return ENXIO;

    uint32_t crc;
    __crc_init (&crc);
    lx_pbx_rec_hdr_t record_header = {
        .magic_bytes = { 'U', 'P', 'X', 'L' },
        /* ignores channel */
        .channel_no  = 0xff,
        .record_type = LX_PBX_RECORD_DRAW_ACCUMULATED
    };
    while ((micros () - driver->last_draw) < 310)
        sched_yield ();
    __lxpbx_write_record_header (&record_header, &crc, driver);
    __crc_send (&crc, driver);
    driver->last_draw = micros ();

    return 0;
}

int lx_pbx_driver_destroy (lx_pbx_driver_t * driver)
{
    if (driver == NULL)
        return EINVAL;
    if (driver->fd < 0)
        return ENXIO;
    serialClose (driver->fd);
    driver->fd = -1;
    return 0;
}

int lx_pbx_open_channel_ws2812 (uint8_t chan_no,
                                lx_pbx_ws2812_chan_t * channel,
                                uint8_t chan_type,
                                uint8_t red_i,
                                uint8_t green_i,
                                uint8_t blue_i,
                                uint8_t white_i)
{
    if (channel == NULL)
        return EINVAL;
    channel->channel_no = chan_no;
    if (ENOTSUP == lx_pbx_set_channel_comp_ws2812 (channel, chan_type))
        return ENOTSUP;
    if ((red_i | green_i | blue_i | white_i) & 0xfc)
        return EDOM;
    channel->red_component_placement   = red_i;
    channel->green_component_placement = green_i;
    channel->blue_component_placement  = blue_i;
    channel->white_component_placement = white_i;

    channel->pixels = 0;

    return 0;
}

int lx_pbx_set_channel_comp_ws2812 (lx_pbx_ws2812_chan_t * channel,
                                    uint8_t chan_type)
{
    if (channel == NULL)
        return EINVAL;
    channel->channel_type = chan_type;
    switch (channel->channel_type) {
        case LX_PBX_CHANNEL_DISABLED: channel->channel_width = 0; break;
        case LX_PBX_CHANNEL_RGB: channel->channel_width = 3; break;
        case LX_PBX_CHANNEL_RGBW: channel->channel_width = 4; break;
        default: return ENOTSUP;
    }

    return 0;
}

void __lxpbx_write_record_header (lx_pbx_rec_hdr_t * rh,
                                  uint32_t * crc,
                                  const lx_pbx_driver_t * driver)
{
    __lxpbx_write (rh, sizeof *rh, crc, driver);
}
void __lxpbx_write_ws2812_chaninfo (lx_pbx_ws2812_chan_t * chaninf,
                                    uint32_t * crc,
                                    const lx_pbx_driver_t * driver)
{
    /* the ws2812_chan_t struct is a bit special because the last two bytes are
     * metadata that shouldn't be sent
     */
    __lxpbx_write (chaninf,
                   4,
                   crc,
                   driver);
}

void __lxpbx_write (const void * data,
                    size_t dlen,
                    uint32_t * crc,
                    const lx_pbx_driver_t * driver)
{
    write (driver->fd, data, dlen);
    *crc = __crc_update (*crc, data, dlen);
}

// Based on https://github.com/simap/PBDriverAdapter/blob/master/src/PBDriverAdapter.cpp

static const uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

uint32_t __crc_update (uint32_t crc, const void * new_data, size_t dlen)
{
    const uint8_t * d = (const unsigned char *)new_data;
    while (dlen--) {
        crc = crc_table[(crc ^ *d) & 0x0f] ^ (crc >> 4);
        crc = crc_table[(crc ^ (*d >> 4)) & 0x0f] ^ (crc >> 4);
        d++;
    }
    return crc;
}

void __crc_init (uint32_t * crc)
{
    *crc = 0xffffffff;
}

void __crc_send (uint32_t * crc,
                 const lx_pbx_driver_t * driver)
{
    (*crc) = (*crc) ^ 0xffffffff;
    write (driver->fd, (uint8_t *)crc, 4);
}
