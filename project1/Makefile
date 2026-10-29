CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -Iinclude
HEADERS = $(wildcard include/*.h)

COMMON_OBJS = build/scanner.o build/compare.o
SYNC_OBJS   = $(COMMON_OBJS) build/main_sync.o
V2_OBJS     = $(COMMON_OBJS) build/logger.o build/metadata.o \
              build/checksum.o build/main_sync_v2.o

all: data_sync data_sync_v2

build/%.o: src/%.c $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

data_sync: $(SYNC_OBJS)
	$(CC) $(SYNC_OBJS) -o $@

data_sync_v2: $(V2_OBJS)
	$(CC) $(V2_OBJS) -o $@

clean:
	rm -rf build data_sync data_sync_v2 sync.log