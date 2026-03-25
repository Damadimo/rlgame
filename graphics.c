#include "graphics.h"

volatile int pixel_buffer_start;
short int Buffer1[240][512];
short int Buffer2[240][512];

void wait_for_vsync(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    int status;

    *pixel_ctrl_ptr = 1;

    // read the status reg at 0xFF20302C
    status = *(pixel_ctrl_ptr + 3);

    // poll S bit of status reg
    while ((status & 0x01) != 0) {
        status = *(pixel_ctrl_ptr + 3);
    }
}

void init_vga_buffers(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;

    // sets Buffer1 as back buffer and then swaps to become front buffer
    *(pixel_ctrl_ptr + 1) = (int)&Buffer1; // stores address of Buffer1 into back buffer
    wait_for_vsync();                       // swap buffers
    pixel_buffer_start = *pixel_ctrl_ptr;   // read front buffer reg to give Buffer1's address
    clear_screen();

    // set Buffer2 as back buffer
    *(pixel_ctrl_ptr + 1) = (int)&Buffer2; // stores address of Buffer2 into back buffer
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // read back buffer to give Buffer2's address
    clear_screen();
}

void plot_pixel(int x, int y, short int color)
{

    // checking bounds (ignore all not within 320x240)
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }

    // each pixel is 2 bytes and each row is 1024 bytes (from lab 7)
    volatile short int *one_pixel_address;
    one_pixel_address = (volatile short int *)(pixel_buffer_start + (y << 10) + (x << 1));
    *one_pixel_address = color;
}

void clear_screen(void)
{
    // use BLACK (0x0000) to clear screen each time 
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            plot_pixel(x, y, BLACK);
        }
    }
}

void draw_rect(int x, int y, int w, int h, short int color)
{
    // iterates from top to bottom row & left to right column to plot pixel
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            plot_pixel(col, row, color);
        }
    }
}

void draw_circle(int cx, int cy, int r, short int color)
{
    // draws a bounding box iterating from -r to r to make a circle
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) { // if this holds, then pixel is within a circle
                plot_pixel(cx + dx, cy + dy, color); // adds the center coordinates than dx, dy which are offsets
            }
        }
    }
}