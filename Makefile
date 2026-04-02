# Host tools (parity / policy smoke). DE1-SoC builds use Makefile.de1 + your BSP flow.
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Ifirmware/shared -Ifirmware/solo -Ifirmware/duel -Iweights

.PHONY: all clean parity policy_smoke policy_duel_smoke

all: tests/parity_steps tests/policy_smoke tests/policy_duel_smoke

parity: tests/parity_steps

policy_smoke: tests/policy_smoke

policy_duel_smoke: tests/policy_duel_smoke

tests/parity_steps: tests/parity_steps.c firmware/solo/game.c firmware/shared/rng.c firmware/shared/input.c tests/graphics_stub.c
	$(CC) $(CFLAGS) -o $@ tests/parity_steps.c firmware/solo/game.c firmware/shared/rng.c firmware/shared/input.c tests/graphics_stub.c

tests/policy_smoke: tests/policy_smoke.c firmware/solo/policy.c
	$(CC) $(CFLAGS) -o $@ tests/policy_smoke.c firmware/solo/policy.c -lm

tests/policy_duel_smoke: tests/policy_duel_smoke.c firmware/duel/policy_duel.c
	$(CC) $(CFLAGS) -o $@ tests/policy_duel_smoke.c firmware/duel/policy_duel.c -lm

clean:
	rm -f tests/parity_steps tests/policy_smoke tests/policy_duel_smoke
