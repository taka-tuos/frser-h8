TARGET	= frser-h8
OBJ		= main.o sci1.o frser.o h8crt0.o

PREFIX	= h8300-elf-

CFLAGS	:= -Os -mh -mint32 -nostartfiles -std=gnu99 -Wall 
ASFLAGS	:= 
LDFLAGS	:= $(CFLAGS) -Wl,-Map=$(TARGET).map -T h8rom.x
LIBS	:= 

include Makefile.in
