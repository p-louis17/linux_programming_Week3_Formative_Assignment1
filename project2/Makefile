CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -Iinclude
HEADERS = $(wildcard include/*.h)
OBJS    = build/logger.o build/fileops.o build/backup_tool.o

all: backup_tool

build/%.o: src/%.c $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

backup_tool: $(OBJS)
	$(CC) $(OBJS) -o $@

clean:
	rm -rf build backup_tool backup.log backup_out