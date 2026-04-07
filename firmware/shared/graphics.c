// DE1 VGA draw helpers, double buffered 320 by 240 active region.

#include "graphics.h"

// Front buffer base the CPU draws through, toggled after each swap.
volatile int pixel_buffer_start;
// Row stride is 512 shorts even though only 320 columns are visible.
short int Buffer1[240][512];
short int Buffer2[240][512];

// Block until the video controller finishes the pending buffer swap.
void wait_for_vsync(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    int status;

    // Start swap then spin until the status S bit drops.
    *pixel_ctrl_ptr = 1;
    status = *(pixel_ctrl_ptr + 3);
    while ((status & 0x01) != 0) {
        status = *(pixel_ctrl_ptr + 3);
    }
}

// Arm both buffers and clear each after its first front buffer assignment.
void init_vga_buffers(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;

    // First half, back buffer points at Buffer1, swap clears it as front.
    *(pixel_ctrl_ptr + 1) = (int)&Buffer1;
    wait_for_vsync();
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen();

    // Second half, draw into Buffer2 while Buffer1 is shown.
    *(pixel_ctrl_ptr + 1) = (int)&Buffer2;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    clear_screen();
}

// Write one pixel in the back buffer, clip if outside the screen.
void plot_pixel(int x, int y, short int color)
{
    // Drop draws outside the 320 by 240 window.
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }

    // y times 1024 bytes per row plus x times two bytes per pixel.
    volatile short int *one_pixel_address;
    one_pixel_address = (volatile short int *)(pixel_buffer_start + (y << 10) + (x << 1));
    *one_pixel_address = color;
}

void clear_screen(void)
{
    // Entire visible window to black.
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            plot_pixel(x, y, BLACK);
        }
    }
}

// Filled axis aligned rectangle, inclusive row and column ranges.
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
    // Test every pixel in the bounding square, keep points inside radius.
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                plot_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}
