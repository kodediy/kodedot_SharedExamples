/*******************************************************************************
 * Size: 20 px
 * Bpp: 1
 * Opts: --bpp 1 --size 20 --no-compress --font InterBold.ttf --symbols @#$%&*!: --range 32-122,48-57 --format lvgl -o Inter_20.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef INTER_20
#define INTER_20 1
#endif

#if INTER_20

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xff, 0xff, 0xfc, 0x7f, 0xc0,

    /* U+0022 "\"" */
    0xcf, 0x3c, 0xf3, 0xcc,

    /* U+0023 "#" */
    0xc, 0x60, 0x63, 0x7, 0x18, 0xff, 0xf7, 0xff,
    0x8c, 0x60, 0x63, 0x3, 0x18, 0xff, 0xf7, 0xff,
    0x8c, 0x70, 0x63, 0x3, 0x18, 0x18, 0xc0,

    /* U+0024 "$" */
    0x6, 0x0, 0xc0, 0xfe, 0x3f, 0xef, 0x7f, 0xcd,
    0xf9, 0x87, 0xf0, 0x7f, 0x83, 0xf8, 0x1f, 0x83,
    0x7e, 0x6f, 0xef, 0xdf, 0xf1, 0xfc, 0x6, 0x0,
    0xc0,

    /* U+0025 "%" */
    0x78, 0x13, 0xf0, 0xcc, 0xc6, 0x33, 0x30, 0xfc,
    0x81, 0xe6, 0x0, 0x30, 0x1, 0x9e, 0x4, 0xfc,
    0x33, 0x31, 0x8c, 0xc4, 0x33, 0x30, 0xfd, 0x81,
    0xe0,

    /* U+0026 "&" */
    0x1e, 0x1, 0xf8, 0x1c, 0xe0, 0xe7, 0x7, 0x78,
    0x1f, 0x80, 0x70, 0xf, 0x8e, 0xfe, 0x77, 0x3b,
    0xb8, 0xf9, 0xe3, 0xc7, 0xfe, 0x1f, 0xb8,

    /* U+0027 "'" */
    0xff, 0xc0,

    /* U+0028 "(" */
    0x38, 0xe7, 0x1c, 0x73, 0x8e, 0x38, 0xe3, 0x8e,
    0x38, 0xe1, 0xc7, 0x1c, 0x38, 0xe0,

    /* U+0029 ")" */
    0x71, 0xc3, 0x8e, 0x38, 0x71, 0xc7, 0x1c, 0x71,
    0xc7, 0x1c, 0xe3, 0x8e, 0x71, 0xc0,

    /* U+002A "*" */
    0x18, 0x18, 0xff, 0x7e, 0x7e, 0xff, 0x18, 0x18,

    /* U+002B "+" */
    0x1c, 0x7, 0x1, 0xc0, 0x70, 0xff, 0xff, 0xf1,
    0xc0, 0x70, 0x1c, 0x7, 0x0,

    /* U+002C "," */
    0x66, 0x66, 0xc0,

    /* U+002D "-" */
    0xff, 0xfc,

    /* U+002E "." */
    0xff, 0x80,

    /* U+002F "/" */
    0x7, 0x6, 0xe, 0xe, 0xc, 0xc, 0x1c, 0x1c,
    0x18, 0x18, 0x38, 0x38, 0x30, 0x70, 0x70, 0x70,
    0x60, 0xe0,

    /* U+0030 "0" */
    0x1f, 0x83, 0xfc, 0x79, 0xe7, 0xe, 0xe0, 0x7e,
    0x7, 0xe0, 0x7e, 0x7, 0xe0, 0x7e, 0x7, 0x70,
    0xe7, 0x9e, 0x3f, 0xc1, 0xf8,

    /* U+0031 "1" */
    0x1c, 0xff, 0xf7, 0x1c, 0x71, 0xc7, 0x1c, 0x71,
    0xc7, 0x1c, 0x70,

    /* U+0032 "2" */
    0x3e, 0x1f, 0xef, 0x3f, 0x87, 0x1, 0xc0, 0x70,
    0x38, 0x1e, 0xf, 0x7, 0x83, 0xc1, 0xe0, 0xff,
    0xff, 0xf0,

    /* U+0033 "3" */
    0x1f, 0x8f, 0xfb, 0xc7, 0x80, 0x70, 0x1e, 0x1f,
    0x3, 0xf0, 0xe, 0x0, 0xe0, 0x1f, 0x83, 0xf8,
    0xf7, 0xfc, 0x3e, 0x0,

    /* U+0034 "4" */
    0x3, 0xc0, 0x7c, 0xf, 0xc0, 0xfc, 0x1d, 0xc3,
    0x9c, 0x39, 0xc7, 0x1c, 0xe1, 0xcf, 0xff, 0xff,
    0xf0, 0x1c, 0x1, 0xc0, 0x1c,

    /* U+0035 "5" */
    0x7f, 0xcf, 0xf9, 0xc0, 0x30, 0xe, 0xf1, 0xff,
    0xbc, 0x70, 0x7, 0x0, 0xe0, 0x1f, 0x83, 0xf8,
    0xe7, 0xfc, 0x3e, 0x0,

    /* U+0036 "6" */
    0x1f, 0x7, 0xf9, 0xc7, 0xb0, 0xe, 0x79, 0xff,
    0xbc, 0x7f, 0x87, 0xe0, 0xfc, 0x1f, 0xc3, 0xb8,
    0xe3, 0xfc, 0x3e, 0x0,

    /* U+0037 "7" */
    0xff, 0xff, 0xf0, 0x1c, 0xe, 0x3, 0x81, 0xc0,
    0x70, 0x38, 0xe, 0x7, 0x1, 0xc0, 0xe0, 0x38,
    0x1c, 0x0,

    /* U+0038 "8" */
    0x1f, 0x87, 0xf9, 0xc7, 0xb8, 0x77, 0x9e, 0x7f,
    0xf, 0xe3, 0x8e, 0xe0, 0xfc, 0x1f, 0x83, 0xf8,
    0xf7, 0xfc, 0x3e, 0x0,

    /* U+0039 "9" */
    0x1f, 0xf, 0xf1, 0xc7, 0x70, 0xfe, 0xf, 0xc3,
    0xfc, 0x7b, 0xff, 0x3e, 0xe0, 0x1f, 0x87, 0x38,
    0xe7, 0xf8, 0x3e, 0x0,

    /* U+003A ":" */
    0xff, 0x80, 0x0, 0xff, 0x80,

    /* U+003B ";" */
    0xff, 0x80, 0x0, 0x1f, 0x6d, 0x80,

    /* U+003C "<" */
    0x0, 0xc0, 0xf1, 0xfd, 0xf8, 0xf8, 0x3c, 0xf,
    0xc0, 0xfe, 0x7, 0xc0, 0x70, 0x4,

    /* U+003D "=" */
    0xff, 0xff, 0xf0, 0x0, 0x0, 0x0, 0x3f, 0xff,
    0xfc,

    /* U+003E ">" */
    0xc0, 0x3c, 0xf, 0xe0, 0x7e, 0x7, 0xc0, 0xf0,
    0xfd, 0xfc, 0xf8, 0x38, 0x8, 0x0,

    /* U+003F "?" */
    0x3f, 0x1f, 0xef, 0x3f, 0x87, 0x1, 0xc1, 0xf0,
    0xf8, 0x78, 0x1c, 0x7, 0x0, 0x0, 0x70, 0x1c,
    0x7, 0x0,

    /* U+0040 "@" */
    0x3, 0xf8, 0x1, 0xff, 0xe0, 0x78, 0x1e, 0x1c,
    0x1, 0xe7, 0x3f, 0xdc, 0xef, 0xf9, 0xfb, 0xcf,
    0x3f, 0x70, 0xe7, 0xee, 0x1c, 0xfd, 0xc3, 0x9f,
    0xb8, 0x73, 0xf7, 0x9e, 0xef, 0x7f, 0xfc, 0xe7,
    0x9f, 0x1e, 0x0, 0x1, 0xf0, 0x20, 0xf, 0xfc,
    0x0, 0x7f, 0x0,

    /* U+0041 "A" */
    0x7, 0xc0, 0x1f, 0x0, 0x7c, 0x3, 0xb8, 0xe,
    0xe0, 0x3b, 0x81, 0xc7, 0x7, 0x1c, 0x3c, 0x70,
    0xff, 0xe3, 0xff, 0x9e, 0xf, 0x70, 0x1d, 0xc0,
    0x70,

    /* U+0042 "B" */
    0xff, 0x9f, 0xfb, 0x87, 0xf0, 0x7e, 0xf, 0xc3,
    0xbf, 0xe7, 0xfe, 0xe1, 0xfc, 0x1f, 0x83, 0xf0,
    0xff, 0xfd, 0xff, 0x0,

    /* U+0043 "C" */
    0xf, 0xc1, 0xff, 0x1e, 0x3c, 0xe0, 0xfe, 0x3,
    0xf0, 0x3, 0x80, 0x1c, 0x0, 0xe0, 0x7, 0x1,
    0xdc, 0x1e, 0xf0, 0xe3, 0xfe, 0x7, 0xe0,

    /* U+0044 "D" */
    0xff, 0xf, 0xfc, 0xe1, 0xee, 0xe, 0xe0, 0x7e,
    0x7, 0xe0, 0x7e, 0x7, 0xe0, 0x7e, 0x7, 0xe0,
    0xee, 0x1e, 0xff, 0xcf, 0xf0,

    /* U+0045 "E" */
    0xff, 0xff, 0xfe, 0x3, 0x80, 0xe0, 0x38, 0xf,
    0xff, 0xff, 0xe0, 0x38, 0xe, 0x3, 0x80, 0xff,
    0xff, 0xf0,

    /* U+0046 "F" */
    0xff, 0xff, 0xfe, 0x3, 0x80, 0xe0, 0x38, 0xf,
    0xfb, 0xfe, 0xe0, 0x38, 0xe, 0x3, 0x80, 0xe0,
    0x38, 0x0,

    /* U+0047 "G" */
    0xf, 0xc1, 0xff, 0x1e, 0x1c, 0xe0, 0xfe, 0x0,
    0x70, 0x3, 0x87, 0xfc, 0x3f, 0xe0, 0x3f, 0x1,
    0xdc, 0x1e, 0xf1, 0xe3, 0xfe, 0x7, 0xe0,

    /* U+0048 "H" */
    0xe0, 0x7e, 0x7, 0xe0, 0x7e, 0x7, 0xe0, 0x7e,
    0x7, 0xff, 0xff, 0xff, 0xe0, 0x7e, 0x7, 0xe0,
    0x7e, 0x7, 0xe0, 0x7e, 0x7,

    /* U+0049 "I" */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xc0,

    /* U+004A "J" */
    0x1, 0xc0, 0x70, 0x1c, 0x7, 0x1, 0xc0, 0x70,
    0x1c, 0x7, 0x1, 0xc0, 0x7e, 0x1f, 0xcf, 0x7f,
    0x8f, 0xc0,

    /* U+004B "K" */
    0xe0, 0xfe, 0x1e, 0xe3, 0xce, 0x78, 0xe7, 0xe,
    0xe0, 0xfe, 0xf, 0xf0, 0xf7, 0xe, 0x78, 0xe3,
    0xce, 0x1c, 0xe1, 0xee, 0xf,

    /* U+004C "L" */
    0xe0, 0x38, 0xe, 0x3, 0x80, 0xe0, 0x38, 0xe,
    0x3, 0x80, 0xe0, 0x38, 0xe, 0x3, 0x80, 0xff,
    0xff, 0xf0,

    /* U+004D "M" */
    0xf0, 0xf, 0xf0, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f,
    0xfc, 0x3f, 0xfc, 0x3f, 0xee, 0x77, 0xee, 0x77,
    0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe3, 0xc7,
    0xe3, 0xc7, 0xe1, 0x87,

    /* U+004E "N" */
    0xe0, 0x7f, 0x7, 0xf0, 0x7f, 0x87, 0xfc, 0x7f,
    0xe7, 0xee, 0x7e, 0x77, 0xe7, 0xfe, 0x3f, 0xe1,
    0xfe, 0xf, 0xe0, 0xfe, 0x7,

    /* U+004F "O" */
    0xf, 0xc0, 0xff, 0xc7, 0x87, 0x9c, 0xe, 0xe0,
    0x1f, 0x80, 0x7e, 0x1, 0xf8, 0x7, 0xe0, 0x1f,
    0x80, 0x77, 0x3, 0x9e, 0x1e, 0x3f, 0xf0, 0x3f,
    0x0,

    /* U+0050 "P" */
    0xff, 0x1f, 0xfb, 0x87, 0xf0, 0x7e, 0xf, 0xc1,
    0xf8, 0x7f, 0xfe, 0xff, 0x1c, 0x3, 0x80, 0x70,
    0xe, 0x1, 0xc0, 0x0,

    /* U+0051 "Q" */
    0xf, 0xc0, 0xff, 0xc7, 0x87, 0x9c, 0xe, 0xe0,
    0x1f, 0x80, 0x7e, 0x1, 0xf8, 0x7, 0xe0, 0x1f,
    0x8e, 0x77, 0x1f, 0x9e, 0x3e, 0x3f, 0xf0, 0x3f,
    0xc0, 0x1, 0x80,

    /* U+0052 "R" */
    0xff, 0xf, 0xfc, 0xe1, 0xee, 0xe, 0xe0, 0xee,
    0xe, 0xe1, 0xef, 0xfc, 0xff, 0x8e, 0x38, 0xe3,
    0xce, 0x1c, 0xe1, 0xee, 0xe,

    /* U+0053 "S" */
    0x1f, 0xf, 0xfb, 0xc7, 0xf0, 0x7e, 0x1, 0xf8,
    0x1f, 0xe1, 0xfe, 0x7, 0xe0, 0x1f, 0x83, 0xf8,
    0xf7, 0xfc, 0x3e, 0x0,

    /* U+0054 "T" */
    0xff, 0xff, 0xff, 0xe, 0x0, 0xe0, 0xe, 0x0,
    0xe0, 0xe, 0x0, 0xe0, 0xe, 0x0, 0xe0, 0xe,
    0x0, 0xe0, 0xe, 0x0, 0xe0,

    /* U+0055 "U" */
    0xe0, 0x7e, 0x7, 0xe0, 0x7e, 0x7, 0xe0, 0x7e,
    0x7, 0xe0, 0x7e, 0x7, 0xe0, 0x7e, 0x7, 0xe0,
    0x77, 0xe, 0x7f, 0xe1, 0xf8,

    /* U+0056 "V" */
    0x70, 0x1d, 0xc0, 0x77, 0x83, 0xce, 0xe, 0x38,
    0x38, 0xf1, 0xe1, 0xc7, 0x7, 0x1c, 0xe, 0xe0,
    0x3b, 0x80, 0xee, 0x1, 0xf0, 0x7, 0xc0, 0x1f,
    0x0,

    /* U+0057 "W" */
    0x70, 0x70, 0x77, 0xf, 0x7, 0x70, 0xf0, 0xf7,
    0x8f, 0x8e, 0x38, 0xf8, 0xe3, 0x9d, 0x8e, 0x39,
    0xdd, 0xc1, 0xdd, 0xdc, 0x1d, 0x8d, 0xc1, 0xf8,
    0xdc, 0x1f, 0x8f, 0x80, 0xf0, 0xf8, 0xf, 0x7,
    0x80, 0xf0, 0x70,

    /* U+0058 "X" */
    0xe0, 0x73, 0x87, 0x9e, 0x38, 0x73, 0x81, 0xfc,
    0xf, 0xc0, 0x3e, 0x1, 0xf0, 0x1f, 0x81, 0xfe,
    0xe, 0x70, 0xf1, 0xc7, 0xf, 0x70, 0x38,

    /* U+0059 "Y" */
    0xe0, 0x3b, 0x83, 0x9c, 0x3c, 0x71, 0xc3, 0xde,
    0xe, 0xe0, 0x7f, 0x1, 0xf0, 0x7, 0x0, 0x38,
    0x1, 0xc0, 0xe, 0x0, 0x70, 0x3, 0x80,

    /* U+005A "Z" */
    0xff, 0xff, 0xfc, 0x7, 0x1, 0xe0, 0x78, 0xe,
    0x3, 0xc0, 0xf0, 0x1c, 0x7, 0x81, 0xe0, 0x38,
    0xf, 0xff, 0xff, 0xc0,

    /* U+005B "[" */
    0xff, 0xfe, 0x38, 0xe3, 0x8e, 0x38, 0xe3, 0x8e,
    0x38, 0xe3, 0x8e, 0x38, 0xff, 0xf0,

    /* U+005C "\\" */
    0xe1, 0xc3, 0x83, 0x7, 0xe, 0x1c, 0x18, 0x38,
    0x70, 0x60, 0xc1, 0xc3, 0x83, 0x6, 0xe,

    /* U+005D "]" */
    0xff, 0xf1, 0xc7, 0x1c, 0x71, 0xc7, 0x1c, 0x71,
    0xc7, 0x1c, 0x71, 0xc7, 0xff, 0xf0,

    /* U+005E "^" */
    0x18, 0x3c, 0x3c, 0x6c, 0x66, 0xe6, 0xc3,

    /* U+005F "_" */
    0xff, 0xff, 0xf0,

    /* U+0060 "`" */
    0xe6, 0x30,

    /* U+0061 "a" */
    0x1f, 0x1f, 0xe7, 0x3c, 0x7, 0xf, 0xdf, 0xff,
    0x1f, 0x87, 0xe3, 0xff, 0xf7, 0xdc,

    /* U+0062 "b" */
    0xe0, 0x1c, 0x3, 0x80, 0x77, 0x8f, 0xfd, 0xe3,
    0xb8, 0x3f, 0x7, 0xe0, 0xfc, 0x1f, 0x83, 0xf8,
    0xef, 0xfd, 0xde, 0x0,

    /* U+0063 "c" */
    0x1f, 0x1f, 0xe7, 0x1f, 0x87, 0xe0, 0x38, 0xe,
    0x3, 0x87, 0x71, 0xdf, 0xe1, 0xf0,

    /* U+0064 "d" */
    0x0, 0xe0, 0x1c, 0x3, 0x8f, 0x77, 0xfe, 0xe3,
    0xf8, 0x3f, 0x7, 0xe0, 0xfc, 0x1f, 0x83, 0xb8,
    0xf7, 0xfe, 0x7d, 0xc0,

    /* U+0065 "e" */
    0x1e, 0x1f, 0xe7, 0x3b, 0x87, 0xff, 0xff, 0xfe,
    0x3, 0x80, 0x73, 0xdf, 0xe1, 0xf0,

    /* U+0066 "f" */
    0x1e, 0x3e, 0x38, 0xfe, 0xfe, 0x38, 0x38, 0x38,
    0x38, 0x38, 0x38, 0x38, 0x38, 0x38,

    /* U+0067 "g" */
    0x1e, 0xef, 0xfd, 0xc7, 0xf0, 0x7e, 0xf, 0xc1,
    0xf8, 0x3f, 0x7, 0x71, 0xef, 0xfc, 0xfb, 0x80,
    0x7f, 0x1e, 0xff, 0x87, 0xc0,

    /* U+0068 "h" */
    0xe0, 0x38, 0xe, 0x3, 0xbc, 0xff, 0xbc, 0xfe,
    0x1f, 0x87, 0xe1, 0xf8, 0x7e, 0x1f, 0x87, 0xe1,
    0xf8, 0x70,

    /* U+0069 "i" */
    0xff, 0x8f, 0xff, 0xff, 0xff, 0xf8,

    /* U+006A "j" */
    0x39, 0xce, 0x3, 0x9c, 0xe7, 0x39, 0xce, 0x73,
    0x9c, 0xe7, 0x3f, 0xb8,

    /* U+006B "k" */
    0xe0, 0x38, 0xe, 0x3, 0x8f, 0xe7, 0xb9, 0xce,
    0xe3, 0xf0, 0xfe, 0x3f, 0x8e, 0xf3, 0x9e, 0xe3,
    0xb8, 0xf0,

    /* U+006C "l" */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xc0,

    /* U+006D "m" */
    0xef, 0x3d, 0xff, 0xfb, 0xce, 0x3f, 0x1c, 0x7e,
    0x38, 0xfc, 0x71, 0xf8, 0xe3, 0xf1, 0xc7, 0xe3,
    0x8f, 0xc7, 0x1f, 0x8e, 0x38,

    /* U+006E "n" */
    0xef, 0x3f, 0xef, 0x3f, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0x87, 0xe1, 0xf8, 0x7e, 0x1c,

    /* U+006F "o" */
    0x1f, 0xf, 0xf1, 0xc7, 0x70, 0x7e, 0xf, 0xc1,
    0xf8, 0x3f, 0x7, 0x71, 0xcf, 0xf8, 0x7c, 0x0,

    /* U+0070 "p" */
    0xef, 0x1f, 0xfb, 0xc7, 0x70, 0x7e, 0xf, 0xc1,
    0xf8, 0x3f, 0x7, 0xf1, 0xdf, 0xfb, 0xbc, 0x70,
    0xe, 0x1, 0xc0, 0x38, 0x0,

    /* U+0071 "q" */
    0x1e, 0xef, 0xfd, 0xc7, 0xf0, 0x7e, 0xf, 0xc1,
    0xf8, 0x3f, 0x7, 0x71, 0xef, 0xfc, 0xfb, 0x80,
    0x70, 0xe, 0x1, 0xc0, 0x38,

    /* U+0072 "r" */
    0xef, 0xff, 0xc7, 0xe, 0x1c, 0x38, 0x70, 0xe1,
    0xc3, 0x80,

    /* U+0073 "s" */
    0x3e, 0x3f, 0xb8, 0xfc, 0xf, 0xc3, 0xf8, 0x7e,
    0x7, 0xe3, 0xff, 0x8f, 0x80,

    /* U+0074 "t" */
    0x38, 0x70, 0xe7, 0xff, 0xe7, 0xe, 0x1c, 0x38,
    0x70, 0xe1, 0xc3, 0xe3, 0xc0,

    /* U+0075 "u" */
    0xe1, 0xf8, 0x7e, 0x1f, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0x87, 0xf3, 0xdf, 0xf3, 0xdc,

    /* U+0076 "v" */
    0x70, 0xee, 0x1d, 0xc7, 0x9c, 0xe3, 0x9c, 0x73,
    0x6, 0xe0, 0xfc, 0x1f, 0x1, 0xe0, 0x3c, 0x0,

    /* U+0077 "w" */
    0x71, 0xc7, 0x38, 0xe3, 0x9c, 0xf9, 0xce, 0x7c,
    0xe3, 0x36, 0x61, 0xdb, 0x70, 0xfd, 0xf8, 0x7e,
    0xfc, 0x1e, 0x3c, 0xf, 0x1e, 0x7, 0x8f, 0x0,

    /* U+0078 "x" */
    0x70, 0xef, 0x38, 0xee, 0xf, 0xc1, 0xf0, 0x1e,
    0x7, 0xc0, 0xfc, 0x3b, 0xce, 0x39, 0xc3, 0x80,

    /* U+0079 "y" */
    0x70, 0xee, 0x1d, 0xc7, 0x9c, 0xe3, 0x9c, 0x73,
    0x6, 0xe0, 0xfc, 0x1f, 0x1, 0xe0, 0x3c, 0x7,
    0x3, 0xe0, 0xf8, 0x1e, 0x0,

    /* U+007A "z" */
    0xff, 0xff, 0xc1, 0xc1, 0xe0, 0xe0, 0xe0, 0xf0,
    0xf0, 0x70, 0x7f, 0xff, 0xe0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 74, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 104, .box_w = 3, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 7, .adv_w = 126, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 11, .adv_w = 208, .box_w = 13, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 34, .adv_w = 210, .box_w = 11, .box_h = 18, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 59, .adv_w = 275, .box_w = 14, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 216, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 67, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 109, .adv_w = 131, .box_w = 6, .box_h = 18, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 123, .adv_w = 131, .box_w = 6, .box_h = 18, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 137, .adv_w = 181, .box_w = 8, .box_h = 8, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 145, .adv_w = 215, .box_w = 10, .box_h = 10, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 158, .adv_w = 97, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 161, .adv_w = 150, .box_w = 7, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 163, .adv_w = 95, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 165, .adv_w = 125, .box_w = 8, .box_h = 18, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 183, .adv_w = 220, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 152, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 202, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 211, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 217, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 206, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 211, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 190, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 332, .adv_w = 211, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 211, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 372, .adv_w = 95, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 377, .adv_w = 96, .box_w = 3, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 383, .adv_w = 215, .box_w = 10, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 215, .box_w = 10, .box_h = 7, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 406, .adv_w = 215, .box_w = 10, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 420, .adv_w = 180, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 438, .adv_w = 330, .box_w = 19, .box_h = 18, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 481, .adv_w = 239, .box_w = 14, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 211, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 526, .adv_w = 241, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 233, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 570, .adv_w = 196, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 588, .adv_w = 187, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 606, .adv_w = 243, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 629, .adv_w = 239, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 650, .adv_w = 90, .box_w = 3, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 656, .adv_w = 182, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 674, .adv_w = 221, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 695, .adv_w = 182, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 713, .adv_w = 296, .box_w = 16, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 741, .adv_w = 235, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 762, .adv_w = 250, .box_w = 14, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 787, .adv_w = 207, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 807, .adv_w = 250, .box_w = 14, .box_h = 15, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 834, .adv_w = 210, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 855, .adv_w = 210, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 875, .adv_w = 214, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 896, .adv_w = 233, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 917, .adv_w = 239, .box_w = 14, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 942, .adv_w = 331, .box_w = 20, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 977, .adv_w = 229, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1000, .adv_w = 232, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1023, .adv_w = 213, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1043, .adv_w = 131, .box_w = 6, .box_h = 18, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 1057, .adv_w = 136, .box_w = 7, .box_h = 17, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1072, .adv_w = 131, .box_w = 6, .box_h = 18, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 1086, .adv_w = 156, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 1093, .adv_w = 152, .box_w = 10, .box_h = 2, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1096, .adv_w = 159, .box_w = 4, .box_h = 3, .ofs_x = 3, .ofs_y = 12},
    {.bitmap_index = 1098, .adv_w = 186, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1112, .adv_w = 203, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1132, .adv_w = 188, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1146, .adv_w = 203, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1166, .adv_w = 191, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1180, .adv_w = 125, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1194, .adv_w = 203, .box_w = 11, .box_h = 15, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 1215, .adv_w = 200, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1233, .adv_w = 87, .box_w = 3, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1239, .adv_w = 87, .box_w = 5, .box_h = 19, .ofs_x = -1, .ofs_y = -4},
    {.bitmap_index = 1251, .adv_w = 186, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1269, .adv_w = 87, .box_w = 3, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1275, .adv_w = 292, .box_w = 15, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1296, .adv_w = 199, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1310, .adv_w = 196, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1326, .adv_w = 202, .box_w = 11, .box_h = 15, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 1347, .adv_w = 202, .box_w = 11, .box_h = 15, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 1368, .adv_w = 137, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1378, .adv_w = 180, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1391, .adv_w = 124, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1404, .adv_w = 199, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1418, .adv_w = 187, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1434, .adv_w = 272, .box_w = 17, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1458, .adv_w = 184, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1474, .adv_w = 187, .box_w = 11, .box_h = 15, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 1495, .adv_w = 183, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 91, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Pair left and right glyphs for kerning*/
static const uint8_t kern_pair_glyph_ids[] =
{
    3, 7,
    3, 13,
    3, 15,
    3, 21,
    3, 34,
    3, 43,
    5, 34,
    7, 3,
    7, 8,
    7, 53,
    7, 55,
    7, 56,
    7, 58,
    7, 87,
    7, 88,
    7, 90,
    8, 7,
    8, 13,
    8, 15,
    8, 21,
    8, 34,
    8, 43,
    9, 68,
    9, 69,
    9, 70,
    9, 72,
    9, 75,
    9, 80,
    11, 7,
    11, 13,
    11, 15,
    11, 21,
    11, 34,
    11, 43,
    11, 64,
    12, 19,
    12, 20,
    12, 24,
    12, 53,
    12, 55,
    12, 56,
    12, 57,
    12, 58,
    12, 89,
    13, 1,
    13, 2,
    13, 3,
    13, 8,
    13, 17,
    13, 18,
    13, 20,
    13, 23,
    13, 25,
    13, 32,
    13, 53,
    13, 58,
    13, 65,
    14, 19,
    14, 20,
    14, 24,
    14, 53,
    14, 55,
    14, 56,
    14, 57,
    14, 58,
    14, 89,
    15, 1,
    15, 2,
    15, 3,
    15, 8,
    15, 17,
    15, 18,
    15, 20,
    15, 23,
    15, 25,
    15, 32,
    15, 53,
    15, 58,
    15, 65,
    16, 13,
    16, 15,
    16, 43,
    16, 55,
    16, 56,
    17, 24,
    17, 58,
    17, 64,
    19, 21,
    20, 13,
    20, 15,
    21, 13,
    21, 15,
    21, 18,
    22, 13,
    22, 15,
    23, 13,
    23, 15,
    23, 64,
    24, 4,
    24, 7,
    24, 13,
    24, 15,
    24, 17,
    24, 20,
    24, 21,
    24, 22,
    24, 23,
    24, 24,
    24, 25,
    24, 26,
    24, 27,
    24, 28,
    24, 29,
    24, 34,
    24, 43,
    24, 64,
    24, 66,
    24, 68,
    24, 69,
    24, 70,
    24, 72,
    24, 80,
    24, 84,
    25, 13,
    25, 15,
    26, 24,
    26, 58,
    26, 64,
    27, 53,
    27, 55,
    28, 53,
    28, 55,
    30, 34,
    30, 55,
    30, 56,
    30, 57,
    30, 58,
    30, 87,
    30, 89,
    30, 90,
    31, 24,
    31, 34,
    31, 53,
    31, 55,
    31, 56,
    31, 57,
    31, 58,
    31, 59,
    32, 34,
    34, 3,
    34, 5,
    34, 8,
    34, 11,
    34, 18,
    34, 29,
    34, 30,
    34, 32,
    34, 36,
    34, 40,
    34, 48,
    34, 50,
    34, 52,
    34, 53,
    34, 55,
    34, 56,
    34, 58,
    34, 63,
    34, 68,
    34, 69,
    34, 70,
    34, 71,
    34, 72,
    34, 74,
    34, 80,
    34, 84,
    34, 85,
    34, 87,
    34, 88,
    34, 90,
    36, 57,
    36, 64,
    36, 87,
    36, 90,
    37, 13,
    37, 15,
    37, 16,
    37, 34,
    37, 53,
    37, 55,
    37, 57,
    37, 58,
    37, 64,
    39, 13,
    39, 15,
    39, 34,
    39, 43,
    39, 64,
    39, 66,
    39, 68,
    39, 69,
    39, 70,
    39, 72,
    39, 78,
    39, 79,
    39, 80,
    39, 81,
    39, 82,
    39, 83,
    39, 86,
    39, 87,
    39, 90,
    39, 91,
    40, 34,
    40, 53,
    40, 55,
    40, 57,
    40, 74,
    40, 75,
    41, 64,
    42, 64,
    43, 13,
    43, 15,
    43, 16,
    43, 64,
    44, 5,
    44, 12,
    44, 14,
    44, 29,
    44, 30,
    44, 36,
    44, 40,
    44, 48,
    44, 50,
    44, 52,
    44, 68,
    44, 69,
    44, 70,
    44, 72,
    44, 74,
    44, 80,
    44, 86,
    44, 87,
    44, 88,
    44, 90,
    45, 3,
    45, 8,
    45, 11,
    45, 12,
    45, 14,
    45, 18,
    45, 34,
    45, 36,
    45, 40,
    45, 48,
    45, 50,
    45, 53,
    45, 55,
    45, 56,
    45, 58,
    45, 63,
    45, 85,
    45, 87,
    45, 90,
    46, 64,
    47, 64,
    48, 13,
    48, 15,
    48, 16,
    48, 34,
    48, 53,
    48, 55,
    48, 57,
    48, 58,
    48, 64,
    49, 7,
    49, 12,
    49, 13,
    49, 14,
    49, 15,
    49, 34,
    49, 43,
    49, 66,
    49, 68,
    49, 69,
    49, 70,
    49, 72,
    49, 80,
    50, 13,
    50, 15,
    50, 16,
    50, 34,
    50, 53,
    50, 55,
    50, 57,
    50, 58,
    50, 64,
    51, 55,
    51, 68,
    51, 69,
    51, 70,
    51, 72,
    51, 80,
    52, 34,
    53, 7,
    53, 12,
    53, 13,
    53, 14,
    53, 15,
    53, 16,
    53, 21,
    53, 27,
    53, 28,
    53, 29,
    53, 34,
    53, 36,
    53, 40,
    53, 43,
    53, 48,
    53, 50,
    53, 64,
    53, 66,
    53, 68,
    53, 69,
    53, 70,
    53, 72,
    53, 78,
    53, 79,
    53, 80,
    53, 81,
    53, 82,
    53, 83,
    53, 84,
    53, 86,
    53, 87,
    53, 88,
    53, 89,
    53, 90,
    53, 91,
    54, 13,
    54, 15,
    54, 16,
    54, 64,
    55, 7,
    55, 12,
    55, 13,
    55, 14,
    55, 15,
    55, 16,
    55, 21,
    55, 27,
    55, 28,
    55, 29,
    55, 30,
    55, 33,
    55, 34,
    55, 36,
    55, 40,
    55, 43,
    55, 48,
    55, 50,
    55, 61,
    55, 64,
    55, 66,
    55, 68,
    55, 69,
    55, 70,
    55, 72,
    55, 80,
    55, 84,
    56, 7,
    56, 12,
    56, 13,
    56, 14,
    56, 15,
    56, 16,
    56, 21,
    56, 27,
    56, 28,
    56, 29,
    56, 30,
    56, 34,
    56, 36,
    56, 40,
    56, 43,
    56, 48,
    56, 50,
    56, 61,
    56, 66,
    56, 68,
    56, 69,
    56, 70,
    56, 72,
    56, 78,
    56, 79,
    56, 80,
    56, 81,
    56, 82,
    56, 83,
    56, 84,
    57, 12,
    57, 14,
    57, 29,
    57, 30,
    57, 36,
    57, 40,
    57, 48,
    57, 50,
    57, 68,
    57, 69,
    57, 70,
    57, 72,
    57, 80,
    58, 7,
    58, 12,
    58, 13,
    58, 14,
    58, 15,
    58, 21,
    58, 27,
    58, 28,
    58, 29,
    58, 30,
    58, 34,
    58, 36,
    58, 40,
    58, 43,
    58, 48,
    58, 50,
    58, 53,
    58, 66,
    58, 68,
    58, 69,
    58, 70,
    58, 72,
    58, 74,
    58, 78,
    58, 79,
    58, 80,
    58, 81,
    58, 82,
    58, 83,
    58, 84,
    58, 86,
    59, 12,
    59, 14,
    59, 29,
    59, 36,
    59, 40,
    59, 48,
    59, 50,
    60, 68,
    60, 69,
    60, 70,
    60, 72,
    60, 75,
    60, 80,
    61, 13,
    61, 15,
    61, 53,
    61, 55,
    61, 56,
    61, 87,
    61, 90,
    63, 7,
    63, 13,
    63, 15,
    63, 21,
    63, 34,
    63, 43,
    63, 64,
    64, 11,
    64, 17,
    64, 18,
    64, 20,
    64, 21,
    64, 22,
    64, 23,
    64, 25,
    64, 26,
    64, 36,
    64, 40,
    64, 48,
    64, 50,
    64, 53,
    64, 54,
    64, 55,
    64, 63,
    64, 74,
    64, 75,
    64, 78,
    64, 79,
    64, 81,
    64, 82,
    64, 83,
    64, 87,
    64, 90,
    66, 18,
    66, 53,
    66, 55,
    66, 56,
    66, 58,
    66, 87,
    66, 90,
    67, 10,
    67, 34,
    67, 53,
    67, 55,
    67, 56,
    67, 57,
    67, 58,
    67, 62,
    67, 87,
    67, 88,
    67, 89,
    67, 90,
    68, 53,
    68, 56,
    68, 58,
    68, 89,
    69, 64,
    70, 53,
    70, 55,
    70, 56,
    70, 58,
    70, 87,
    70, 88,
    70, 89,
    70, 90,
    71, 12,
    71, 13,
    71, 14,
    71, 15,
    71, 16,
    71, 21,
    71, 34,
    71, 43,
    71, 58,
    71, 64,
    71, 66,
    71, 68,
    71, 69,
    71, 70,
    71, 72,
    71, 80,
    71, 84,
    72, 53,
    72, 64,
    72, 75,
    73, 18,
    73, 53,
    73, 55,
    73, 56,
    73, 58,
    73, 87,
    73, 90,
    74, 10,
    74, 55,
    74, 56,
    74, 62,
    74, 64,
    75, 10,
    75, 55,
    75, 56,
    75, 62,
    75, 64,
    76, 12,
    76, 14,
    76, 21,
    76, 29,
    76, 53,
    76, 68,
    76, 69,
    76, 70,
    76, 72,
    76, 80,
    76, 87,
    76, 90,
    77, 64,
    78, 18,
    78, 53,
    78, 55,
    78, 56,
    78, 58,
    78, 87,
    78, 90,
    79, 18,
    79, 53,
    79, 55,
    79, 56,
    79, 58,
    79, 87,
    79, 90,
    80, 10,
    80, 34,
    80, 53,
    80, 55,
    80, 56,
    80, 57,
    80, 58,
    80, 62,
    80, 87,
    80, 88,
    80, 89,
    80, 90,
    81, 10,
    81, 34,
    81, 53,
    81, 55,
    81, 56,
    81, 57,
    81, 58,
    81, 62,
    81, 87,
    81, 88,
    81, 89,
    81, 90,
    82, 10,
    82, 34,
    82, 53,
    82, 55,
    82, 56,
    82, 57,
    82, 58,
    82, 62,
    82, 87,
    82, 88,
    82, 89,
    82, 90,
    83, 12,
    83, 13,
    83, 14,
    83, 15,
    83, 16,
    83, 29,
    83, 34,
    83, 43,
    83, 53,
    83, 59,
    83, 66,
    83, 68,
    83, 69,
    83, 70,
    83, 71,
    83, 72,
    83, 80,
    83, 85,
    83, 87,
    83, 88,
    83, 89,
    83, 90,
    84, 53,
    84, 55,
    84, 56,
    84, 58,
    85, 12,
    85, 14,
    85, 21,
    85, 29,
    85, 35,
    85, 37,
    85, 38,
    85, 39,
    85, 41,
    85, 42,
    85, 44,
    85, 45,
    85, 46,
    85, 47,
    85, 49,
    85, 51,
    85, 53,
    85, 55,
    85, 56,
    85, 58,
    85, 67,
    85, 68,
    85, 69,
    85, 70,
    85, 72,
    85, 73,
    85, 76,
    85, 77,
    85, 80,
    85, 84,
    86, 53,
    86, 58,
    87, 13,
    87, 15,
    87, 16,
    87, 29,
    87, 30,
    87, 34,
    87, 43,
    87, 53,
    87, 58,
    87, 64,
    87, 66,
    87, 68,
    87, 69,
    87, 70,
    87, 72,
    87, 74,
    87, 80,
    88, 7,
    88, 13,
    88, 15,
    88, 34,
    88, 43,
    88, 53,
    88, 59,
    88, 68,
    88, 69,
    88, 70,
    88, 72,
    88, 80,
    89, 12,
    89, 14,
    89, 30,
    89, 53,
    89, 66,
    89, 68,
    89, 69,
    89, 70,
    89, 72,
    89, 80,
    90, 13,
    90, 15,
    90, 16,
    90, 29,
    90, 30,
    90, 34,
    90, 43,
    90, 53,
    90, 58,
    90, 64,
    90, 66,
    90, 68,
    90, 69,
    90, 70,
    90, 72,
    90, 74,
    90, 80,
    91, 53,
    91, 68,
    91, 69,
    91, 70,
    91, 72,
    91, 80
};

/* Kerning between the respective left and right glyphs
 * 4.4 format which needs to scaled with `kern_scale`*/
static const int8_t kern_pair_values[] =
{
    -11, -29, -29, -20, -25, -53, -5, -11,
    -11, -18, -20, -13, -24, -11, -11, -11,
    -11, -29, -29, -20, -25, -53, -1, -1,
    -1, -1, 4, -1, -11, -44, -44, -15,
    -31, -31, -22, -7, -1, -9, -9, -7,
    -7, -8, -10, -7, -11, -4, -32, -32,
    -13, -25, -11, -13, -11, -34, -11, -22,
    -44, -7, -1, -9, -9, -7, -7, -8,
    -10, -7, -11, -4, -32, -32, -13, -25,
    -11, -13, -11, -34, -11, -22, -44, -13,
    -13, -20, 3, 3, -6, -11, -15, -5,
    -11, -11, -11, -11, -3, -11, -11, -11,
    -11, -15, -18, -15, -40, -40, -5, -5,
    -19, -3, -5, 6, -5, -3, -11, -11,
    -29, -35, -31, -51, -15, -16, -16, -16,
    -16, -16, -13, -11, -11, -6, -11, -15,
    -11, -20, -11, -20, -16, -22, -20, -16,
    -31, -13, -11, -13, -24, -14, -25, -24,
    -24, -18, -29, -20, -20, -25, -5, -25,
    -31, -11, -14, -16, -20, -11, -11, -11,
    -11, -5, -29, -29, -24, -24, -31, -8,
    -8, -8, -3, -8, 1, -8, -4, -11,
    -22, -22, -22, -5, -13, 2, 2, -15,
    -15, -13, -11, -10, -9, -11, -14, -13,
    -13, -11, -29, -22, -11, -2, -13, -13,
    -13, -13, -3, -3, -13, -3, -3, -3,
    -11, -11, -11, -11, -7, 11, -7, -5,
    11, 11, 11, 11, -11, -11, -18, -16,
    -2, -14, -14, -22, -11, -13, -13, -13,
    -13, -2, -13, -13, -13, -13, 11, -13,
    -11, -13, -20, -13, -29, -29, -13, -11,
    -16, -15, 6, -9, -9, -9, -9, -29,
    -22, -13, -30, -42, -7, -22, -22, 11,
    11, -15, -15, -13, -11, -10, -9, -11,
    -14, -13, -11, -6, -13, -6, -11, -27,
    -31, -5, -3, -3, -3, -3, -3, -15,
    -15, -13, -11, -10, -9, -11, -14, -13,
    -7, -3, -3, -3, -3, -3, -5, -11,
    -9, -11, -16, -11, -25, -22, -11, -11,
    -25, -29, -10, -10, -29, -10, -10, -18,
    -24, -25, -25, -25, -25, -18, -18, -25,
    -18, -18, -18, -24, -24, -20, -20, -9,
    -20, -15, -11, -11, -18, -16, -16, -7,
    -31, -7, -31, -21, -11, -20, -20, -24,
    -20, -11, -31, -9, -9, -31, -9, -9,
    3, -25, -19, -20, -20, -20, -20, -20,
    -15, -16, -7, -31, -7, -31, -11, -11,
    -16, -16, -24, -20, -24, -11, -11, -20,
    -11, -11, 3, -19, -19, -19, -19, -19,
    -11, -11, -19, -11, -11, -11, -11, -9,
    -9, -18, -16, -11, -11, -11, -11, -9,
    -9, -9, -9, -9, -20, -10, -22, -10,
    -22, -22, -20, -20, -33, -31, -24, -14,
    -14, -11, -14, -14, 15, -34, -36, -36,
    -36, -36, -3, -13, -13, -36, -13, -13,
    -13, -22, -13, -5, -5, -20, -11, -11,
    -11, -11, -1, -1, -1, -1, 4, -1,
    13, 13, -13, -18, -11, -16, -16, -11,
    -44, -44, -15, -31, -31, -22, -22, -15,
    -35, -15, -18, -15, -15, -15, -15, -13,
    -13, -13, -13, -18, -13, -25, -22, 11,
    31, 15, 15, 15, 15, 15, -25, -25,
    -13, -15, -16, -16, -32, -6, -6, -4,
    -11, -13, -20, -16, -9, -37, -4, -6,
    -4, -6, -6, -24, -9, -20, -1, 11,
    -16, -16, -16, -35, -3, -1, -3, -3,
    -6, -18, -6, -18, -4, -20, -11, -20,
    1, -5, -7, -6, -6, -6, -6, -6,
    -6, -16, 11, 3, -13, -15, -16, -16,
    -32, -6, -6, 11, 11, 11, 11, 13,
    11, 11, 11, 11, 13, -6, -6, -15,
    -27, -18, -7, -7, -7, -7, -7, 3,
    3, 11, -13, -15, -16, -16, -32, -6,
    -6, -13, -15, -16, -16, -32, -6, -6,
    -4, -11, -13, -20, -16, -9, -37, -4,
    -6, -4, -6, -6, -4, -11, -13, -20,
    -16, -9, -37, -4, -6, -4, -6, -6,
    -4, -11, -13, -20, -16, -9, -37, -4,
    -6, -4, -6, -6, -5, -20, -5, -20,
    -10, -13, -13, -18, -18, -11, -5, -8,
    -8, -8, 5, -8, -8, 11, 3, 3,
    3, 3, -24, -15, -11, -24, -5, -5,
    -2, -9, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, -11, -1,
    -1, -2, 4, -2, -2, -2, -2, 4,
    4, 4, -2, -2, -16, -18, -11, -11,
    -18, -11, -13, -22, -20, -11, -11, -25,
    -3, -6, -6, -6, -6, 5, -6, -11,
    -24, -24, -22, -18, -16, -11, -4, -4,
    -4, -4, -4, -7, -7, -11, -9, -1,
    -6, -6, -6, -6, -6, -11, -11, -18,
    -11, -13, -22, -20, -11, -11, -25, -3,
    -6, -6, -6, -6, 5, -6, -11, -5,
    -5, -5, -5, -5
};

/*Collect the kern pair's data in one place*/
static const lv_font_fmt_txt_kern_pair_t kern_pairs =
{
    .glyph_ids = kern_pair_glyph_ids,
    .values = kern_pair_values,
    .pair_cnt = 748,
    .glyph_ids_size = 0
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_pairs,
    .kern_scale = 16,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t Inter_20 = {
#else
lv_font_t Inter_20 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -4,
    .underline_thickness = 2,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if INTER_20*/

