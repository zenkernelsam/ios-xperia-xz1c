#pragma once
#include <stdint.h>

#define FB_BASE		0x9d400000UL	/* reserved-memory cont_splash_region */
#define FB_W		720
#define FB_H		1280
/* lilac panel consumes RGB888 through the DSI path (qcom,mdss-dsi-bpp=24):
   720 px * 3 B = 2160 B per scanout line. Writing 16bpp made every glyph
   row appear ~1.5x wider and folded rows; 32bpp caused diagonal shear. */
#define FB_STRIDE	(FB_W * 3)	/* 2160 bytes per line */
#define FB_GS		(FB_STRIDE / 3)	/* pixels per FB line (=720) */

#define RED	0xffff0000U
#define BLUE	0xff0000a0U
#define WHITE	0xffffffffU
#define YELLOW	0xffffff00U
#define GREEN	0xff00ff00U

extern volatile uint8_t *con_fb;

void con_clear(uint32_t bg);
void con_skip(void);
void con_str(const char *label, const char *s);
void con_hex(const char *label, uint64_t v);
/* label, then "a=xxxxxxxx b=xxxxxxxx" style pairs of 32-bit values */
void con_hex2(const char *label, const char *a, uint32_t va, const char *b, uint32_t vb);

char *con_log(uint32_t *len);
