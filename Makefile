# Host tools (parity / policy smoke). DE1-SoC builds use your usual BSP flow.
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -I.

.PHONY: all clean parity policy_smoke

all: tests/parity_steps tests/policy_smoke

parity: tests/parity_steps

policy_smoke: tests/policy_smoke

tests/parity_steps: tests/parity_steps.c game.c rng.c input.c tests/graphics_stub.c
	$(CC) $(CFLAGS) -o $@ tests/parity_steps.c game.c rng.c input.c tests/graphics_stub.c

tests/policy_smoke: tests/policy_smoke.c policy.c
	$(CC) $(CFLAGS) -o $@ tests/policy_smoke.c policy.c -lm

clean:
	rm -f tests/parity_steps tests/policy_smoke
