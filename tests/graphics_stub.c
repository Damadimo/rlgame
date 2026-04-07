// Host build has no VGA hardware so these are empty shells for linking game.c.
// Parity tests only care about game logic, not pixels.

#include "graphics.h"

// Same symbols as firmware/shared/graphics.c so the linker resolves draw calls.
volatile int pixel_buffer_start;
short int Buffer1[240][512];
short int Buffer2[240][512];

// No controller to wait on when running under gcc on a laptop.
void wait_for_vsync(void) {}

void init_vga_buffers(void)
{
    // Any non zero base is enough because plot_pixel never runs in these tests.
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
