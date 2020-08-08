//
// file pbx.h
// author Maximilien M. Cura
//

#ifndef __LX_PBX_DRIVER
#define __LX_PBX_DRIVER

#include <stddef.h> /* size_t */
#include <stdint.h> /* [u]intN_t */

#define LX_PBX_BAUD_RATE 2000000L

#define LX_PBX_RECORD_CHANNEL_WS2812 1
#define LX_PBX_RECORD_DRAW_ACCUMULATED 2

typedef struct lx_pbx_rec_hdr
{
    int8_t magic_bytes[4];
    uint8_t channel_no;
    uint8_t record_type;
} lx_pbx_rec_hdr_t;

#define LX_PBX_CHANNEL_DISABLED 0
#define LX_PBX_CHANNEL_RGB 3
#define LX_PBX_CHANNEL_RGBW 4

typedef struct lx_pbx_ws2812_chan
{
    uint8_t channel_type;
    uint8_t red_component_placement : 2;
    uint8_t green_component_placement : 2;
    uint8_t blue_component_placement : 2;
    uint8_t white_component_placement : 2;
    uint16_t pixels;
    /* metadata ; do not send */
    uint8_t channel_no;
    uint8_t channel_width;
} lx_pbx_ws2812_chan_t;

typedef struct lx_pbx_driver
{
    int fd;
    unsigned long last_draw;
} lx_pbx_driver_t;

#define LX_PBX_NO_DRIVER -1

int lx_pbx_init (int needs_wpi_setup);
int lx_pbx_driver_create (const char * device_file,
                          lx_pbx_driver_t * driver);
int lx_pbx_driver_write_ws2812_chan (const lx_pbx_driver_t * driver,
                                     lx_pbx_ws2812_chan_t * channel,
                                     uint8_t * pixel_data,
                                     size_t pixel_num);
int lx_pbx_driver_draw_accumulated (lx_pbx_driver_t * driver);
int lx_pbx_driver_destroy (lx_pbx_driver_t * driver);

int lx_pbx_open_channel_ws2812 (uint8_t chan_no,
                                lx_pbx_ws2812_chan_t * channel,
                                uint8_t chan_type,
                                uint8_t red_i,
                                uint8_t green_i,
                                uint8_t blue_i,
                                uint8_t white_i);
int lx_pbx_set_channel_comp_ws2812 (lx_pbx_ws2812_chan_t * channel,
                                    uint8_t chan_type);

#endif /* !@__LX_PBX_DRIVER */
