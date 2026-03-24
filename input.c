#include "input.h"

static InputMode mode = INPUT_MODE_HUMAN;
static int agent_keys = 0;

int read_keys(void)
{
    volatile int *key_ptr = (int *)KEY_BASE;
    return *key_ptr;
}

void input_set_mode(InputMode m)
{
    mode = m;
}

void input_set_agent_keys(int key_bits)
{
    agent_keys = key_bits;
}

int read_movement_keys(void)
{
    if (mode == INPUT_MODE_AGENT) {
        return agent_keys;
    }
    return read_keys();
}
