// DE1 key GPIO versus software injected key bits for player movement.

#include "input.h"

// Human mode reads hardware keys, agent mode uses agent_keys only for movement.
static InputMode mode = INPUT_MODE_HUMAN;
static int agent_keys = 0;

// Raw pushbutton register read from the DE1 memory map.
int read_keys(void)
{
    volatile int *key_ptr = (int *)KEY_BASE;
    return *key_ptr;
}

void input_set_mode(InputMode m)
{
    // Switches whether read_movement_keys hits hardware or agent_keys.
    mode = m;
}

// Agent mode ignores GPIO for movement and uses the bits set here instead.
void input_set_agent_keys(int key_bits)
{
    agent_keys = key_bits;
}

// Solo basket and duel right pane use this entry point.
int read_movement_keys(void)
{
    if (mode == INPUT_MODE_AGENT) {
        return agent_keys;
    }
    return read_keys();
}

// Duel left side always uses this so the human never steals agent injection.
int read_human_movement_keys(void)
{
    return read_keys();
}

// Duel right pane reads the last bits main or the policy path injected.
int input_get_agent_keys(void)
{
    return agent_keys;
}
