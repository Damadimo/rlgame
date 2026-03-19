#ifndef GRAPHICS_H
#define GRAPHICS_H

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define PIXEL_BUF_CTRL_BASE 0xFF203020

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define MAGENTA 0xF81F
#define CYAN    0x07FF
#define ORANGE  0xFC00
#define BROWN   0xA145
#define GRAY    0x8410

extern volatile int pixel_buffer_start;
extern short int Buffer1[240][512];
extern short int Buffer2[240][512];

void init_vga_buffers(void);
void wait_for_vsync(void);
void plot_pixel(int x, int y, short int color);
void clear_screen(void);
void draw_rect(int x, int y, int w, int h, short int color);
void draw_circle(int cx, int cy, int r, short int color);

#endif