/* Host-only stubs for linking game.c without DE1 hardware. */
#include "graphics.h"

volatile int pixel_buffer_start;
short int Buffer1[240][512];
short int Buffer2[240][512];

void wait_for_vsync(void) {}

void init_vga_buffers(void)
{
    pixel_buffer_start = (int)(long)Buffer1;
}

void plot_pixel(int x, int y, short int color)
{
    (void)x;
    (void)y;
    (void)color;
}

void clear_screen(void) {}

void draw_rect(int x, int y, int w, int h, short int color)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
}

void draw_circle(int cx, int cy, int r, short int color)
{
    (void)cx;
    (void)cy;
    (void)r;
    (void)color;
}
