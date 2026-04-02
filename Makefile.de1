# DE1-SoC / RISC-V build (Intel Quartus + riscv32-unknown-elf-gcc on Windows).
#
# --- Unified (default): menu — AI solo | split | human solo | AI solo (alt weights); post-game menu/again ---
#   firmware/main_game.c links solo + policy_alt + duel; default solo policy is int8
#   (weights/policy_weights.h -> policy.c). Alt is float MLP in weights/policy_weights_alt.h
#   (policy_alt.c; 3-D input packed from full obs in policy_alt_select_action_from_game_obs).
#   Duel uses weights/policy_weights_duel.h -> policy_duel.c.
#
# --- Solo-only (AI full screen) ---
#   MAIN := firmware/solo/main_agent.c  TARGET := basket_game_ai
#   SRCS := graphics input rng game observation policy (no duel_*, no policy_alt).
#
# --- Duel-only (split human vs AI) ---
#   MAIN := firmware/duel/main_duel.c
#   SRCS := graphics input rng duel_game observation_duel policy_duel (no solo game/observation/policy/policy_alt).
#
# Host tests (parity, policy_smoke) use the root Makefile, not this file.

INSTALL := C:/intelFPGA/QUARTUS_Lite_V23.1

MAIN    := firmware/main_game.c
TARGET  := basket_game
INCLUDES := -Ifirmware/shared -Ifirmware/solo -Ifirmware/duel -Iweights
HDRS    := firmware/shared/graphics.h firmware/shared/input.h firmware/shared/rng.h \
           firmware/solo/game.h firmware/solo/observation.h firmware/solo/policy.h firmware/solo/policy_alt.h \
           firmware/duel/duel_game.h firmware/duel/observation_duel.h firmware/duel/policy_duel.h \
           weights/policy_weights.h weights/policy_weights_alt.h weights/policy_weights_duel.h
SRCS    := firmware/main_game.c firmware/shared/graphics.c firmware/solo/game.c firmware/duel/duel_game.c \
           firmware/shared/input.c firmware/shared/rng.c firmware/solo/observation.c firmware/solo/policy.c \
           firmware/solo/policy_alt.c \
           firmware/duel/observation_duel.c firmware/duel/policy_duel.c

SHELL   := cmd.exe

# DE1-SoC
JTAG_INDEX_SoC := 2

# The following variables are set based on the value of the INSTALL variable
COMPILER        := $(INSTALL)/fpgacademy/AMP/cygwin64/home/compiler/bin
BASH            := $(INSTALL)/fpgacademy/AMP/cygwin64/bin/bash --noprofile -norc -c
HW_DE1-SoC      := "$(INSTALL)/fpgacademy/Computer_Systems/DE1-SoC/DE1-SoC_Computer/niosVg/DE1_SoC_Computer.sof"
HW_DE10-Lite    := "$(INSTALL)/fpgacademy/Computer_Systems/DE10-Lite/DE10-Lite_Computer/niosVg/DE10_Lite_Computer.sof"

# for Quartus programmer (two possibilities exist for the path)
export PATH := $(INSTALL)/quartus/bin64/:$(PATH)
export PATH := $(INSTALL)/qprogrammer/quartus/bin64/:$(PATH)
# for GDB server
export PATH := $(INSTALL)/riscfree/debugger/gdbserver-riscv/:$(PATH)
# for GDB client
export PATH := $(INSTALL)/riscfree/toolchain/riscv32-unknown-elf/bin/:$(PATH)
# for the nios2-terminal
export PATH := $(INSTALL)/fpgacademy/AMP/bin/:$(PATH)
# for checking JTAG chain
export PATH := $(INSTALL)/quartus/sopc_builder/bin/:$(PATH)

CYGWIN_INSTALL := $(shell $(BASH) 'export PATH=/usr/local/bin:/usr/bin; cygpath $(INSTALL)')
CYGWIN_PATH := export PATH=/usr/local/bin:/usr/bin:$(CYGWIN_INSTALL)/fpgacademy/AMP/bin

# Programs
CC  := $(COMPILER)/riscv32-unknown-elf-gcc.exe
LD  := $(CC)
OD  := $(COMPILER)/riscv32-unknown-elf-objdump.exe
NM  := $(COMPILER)/riscv32-unknown-elf-nm.exe
RM  := /usr/bin/rm -f

# Flags (match host Makefile: C11 + same include roots)
USERCCFLAGS := -std=c11 -g -O1 -ffunction-sections -fverbose-asm -fno-inline -gdwarf-2
USERLDFLAGS := -Wl,--defsym=__stack_pointer$$=0x4000000 -Wl,--defsym -Wl,JTAG_UART_BASE=0xff201000 -lm
ARCHCCFLAGS := -march=rv32im_zicsr -mabi=ilp32
ARCHLDFLAGS := -march=rv32im_zicsr -mabi=ilp32
CCFLAGS     := -Wall -c $(USERCCFLAGS) $(ARCHCCFLAGS) $(INCLUDES)
LDFLAGS     := $(USERLDFLAGS) $(ARCHLDFLAGS)

# Files
OBJS        := $(SRCS:.c=.o)

############################################
# GDB Macros

# Programs
GDB_SERVER      := ash-riscv-gdb-server.exe
GDB_CLIENT      := riscv32-unknown-elf-gdb.exe

############################################
# System Macros

# Programs
QP_PROGRAMMER   := quartus_pgm.exe

# Flags
# DE10-Lite
SYS_FLAG_CABLE_Lite     := -c "USB-Blaster [USB-0]"
# DE1-SoC
SYS_FLAG_CABLE_SoC      := -c "DE-SoC [USB-1]"
# quartus_pgm cable flag for DETECT_DEVICES (default: DE1-SoC; override when using DE10-Lite flow)
SYS_FLAG_CABLE          := $(SYS_FLAG_CABLE_SoC)

# DE10-Lite
JTAG_INDEX_Lite := 1
RED_TEXT        := @$(BASH) 'printf "\033[31m"'
GREEN_TEXT      := @$(BASH) 'printf "\033[32m"'
CYAN_TEXT       := @$(BASH) 'printf "\033[36m"'
YELLOW_TEXT     := @$(BASH) 'printf "\033[33m"'
DEF_TEXT        := @$(BASH) 'printf "\033[0m"'

############################################
# Compilation Targets

COMPILE: $(TARGET).elf

$(TARGET).elf: $(OBJS)
	@$(BASH) 'cd "$(CURDIR)"; $(RM) $@'
	$(CYAN_TEXT)
	@echo Linking
	@$(BASH) 'printf "$(LD) "'
	$(DEF_TEXT)
	@echo $(LDFLAGS) $(OBJS) -o $@
	@$(BASH) 'printf "\n"'
	@$(BASH) 'cd "$(CURDIR)"; $(CYGWIN_PATH); $(LD) $(LDFLAGS) $(OBJS) -o $@'

%.o: %.c $(HDRS)
	@$(BASH) 'cd "$(CURDIR)"; $(RM) $@'
	$(GREEN_TEXT)
	@echo Compiling
	@$(BASH) 'printf "$(CC) "'
	$(DEF_TEXT)
	@echo $(CCFLAGS) $< -o $@
	@$(BASH) 'cd "$(CURDIR)"; $(CYGWIN_PATH); $(CC) $(CCFLAGS) $< -o $@'

SYMBOLS: $(TARGET).elf
	@echo $(NM) -p $<
	@$(BASH) 'cd "$(CURDIR)"; $(CYGWIN_PATH); $(NM) -p $<'

OBJDUMP: $(TARGET).elf
	@echo $(OD) -d -S $<
	@$(BASH) 'cd "$(CURDIR)"; $(CYGWIN_PATH); $(OD) -d -S $<'

CLEAN:
	$(RED_TEXT)
	@$(BASH) 'printf "$(RM) "'
	$(DEF_TEXT)
	@echo $(TARGET).elf $(OBJS)
	@$(BASH) 'cd "$(CURDIR)"; $(RM) $(TARGET).elf $(OBJS)'

############################################
# System Targets

DETECT_DEVICES:
	$(QP_PROGRAMMER) $(SYS_FLAG_CABLE) --auto

DE1-SoC:
	$(QP_PROGRAMMER) $(SYS_FLAG_CABLE_SoC) -m jtag -o "P;$(HW_DE1-SoC)@$(JTAG_INDEX_SoC)"

DE10-Lite:
	$(QP_PROGRAMMER) $(SYS_FLAG_CABLE_Lite) -m jtag -o "P;$(HW_DE10-Lite)@$(JTAG_INDEX_Lite)"

TERMINAL:
	nios2-terminal.exe --instance 0

############################################
# GDB Targets

GDB_SERVER:
	$(GDB_SERVER) --device 02D120DD --gdb-port 2454 --instance 1 --probe-type USB-Blaster-2 --transport-type jtag --auto-detect true

GDB_CLIENT:
	$(GDB_CLIENT) -silent -ex "target remote:2454" -ex "set $$mstatus=0" -ex "set $$mtvec=0" -ex "load" -ex "set $$pc=_start" -ex "info reg pc" "$(TARGET).elf"

############################################
# EXTRAS

.SILENT: SYMBOLS OBJDUMP
