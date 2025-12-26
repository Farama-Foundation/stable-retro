TC_PREFIX  = /home/swordfish/Downloads/gcc-arm-10.2-2020.11-x86_64-arm-none-eabi
PREFIX     = $(TC_PREFIX)
AS         = $(PREFIX)/bin/arm-none-eabi-gcc
OBJCOPY    = $(PREFIX)/bin/arm-none-eabi-objcopy

BIN_ARM7   = drastic_bios_arm7
BIN_ARM9   = drastic_bios_arm9

all:
	$(AS) bios_common.S -DBIOS_ARM7 -march=armv4t -c -Wa,-asl=$(BIN_ARM7).list -o $(BIN_ARM7).o
	$(AS) bios_common.S -DBIOS_ARM9 -march=armv5t -c -Wa,-asl=$(BIN_ARM9).list -o $(BIN_ARM9).o
	$(OBJCOPY) -O binary $(BIN_ARM7).o $(BIN_ARM7).bin
	$(OBJCOPY) -O binary $(BIN_ARM9).o $(BIN_ARM9).bin

clean:
	rm -f $(BIN_ARM7).bin $(BIN_ARM7).o $(BIN_ARM9).bin $(BIN_ARM9).o

