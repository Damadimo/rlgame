#ifndef INPUT_H
#define INPUT_H

#define KEY_BASE 0xFF200050

typedef enum {
    INPUT_MODE_HUMAN = 0,
    INPUT_MODE_AGENT = 1
} InputMode;

int read_keys(void);
int read_movement_keys(void);
/** Always reads hardware keys (for human in split-screen duel; ignores agent injection). */
int read_human_movement_keys(void);
/** Last bits passed to input_set_agent_keys (for AI pane in duel). */
int input_get_agent_keys(void);
void input_set_mode(InputMode mode);
void input_set_agent_keys(int key_bits);

#endif
