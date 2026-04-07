#include "input.h"

int read_keys(void)
{
    volatile int *key_ptr = (int *)KEY_BASE;
    return *key_ptr;
}