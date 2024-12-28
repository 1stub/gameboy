CC = gcc
CFLAGS = -Wall -Wextra -g -std=c23 $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)
TARGET = gameboy 
SRCS = mmu.c cpu.c debugger.c display.c ppu.c interrupt.c timer.c emulator.c main.c   
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

