#include "graphics.h"

volatile int pixel_buffer_start;
short int Buffer1[240][512];
short int Buffer2[240][512];

void wait_for_vsync(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    int status;

    *pixel_ctrl_ptr = 1;
    status = *(pixel_ctrl_ptr + 3);

    while ((status & 0x01) != 0) {
        status = *(pixel_ctrl_ptr + 3);
    }
}

void init_vga_buffers(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;

    *(pixel_ctrl_ptr + 1) = (int)&Buffer1;
    wait_for_vsync();
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen();

    *(pixel_ctrl_ptr + 1) = (int)&Buffer2;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    clear_screen();
}

void plot_pixel(int x, int y, short int color)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }

    volatile short int *one_pixel_address;
    one_pixel_address = (volatile short int *)(pixel_buffer_start + (y << 10) + (x << 1));
    *one_pixel_address = color;
}

void clear_screen(void)
{
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            plot_pixel(x, y, BLACK);
        }
    }
}

void draw_rect(int x, int y, int w, int h, short int color)
{
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            plot_pixel(col, row, color);
        }
    }
}

void draw_circle(int cx, int cy, int r, short int color)
{
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                plot_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}