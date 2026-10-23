#pragma once
#include <stdint.h>

#define FB_BASE		0x9d400000UL	/* reserved-memory cont_splash_region */
#define FB_W		720
#define FB_H		1280
/* Android fb0 reports the MDP scanout as 32bpp stride 2944 (720px +
   64B pad to 128B). Pixels are XRGB8888; the rightmost 16 px per line
   is padding — draw only FB_W columns. */
#define FB_STRIDE	2944UL
#define FB_GS		(FB_STRIDE / 4)	/* 736 dwords per line */

#define RED	0xffff0000U
#define BLUE	0xff0000a0U
#define WHITE	0xffffffffU
#define YELLOW	0xffffff00U
#define GREEN	0xff00ff00U

extern volatile uint32_t *con_fb;

void con_clear(uint32_t bg);
void con_skip(void);
void con_str(const char *label, const char *s);
void con_hex(const char *label, uint64_t v);
/* label, then "a=xxxxxxxx b=xxxxxxxx" style pairs of 32-bit values */
void con_hex2(const char *label, const char *a, uint32_t va, const char *b, uint32_t vb);

char *con_log(uint32_t *len);
