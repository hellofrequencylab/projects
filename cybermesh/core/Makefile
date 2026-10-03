# Host build of the floor core: unit tests plus the simulated mesh.
CC ?= cc
CFLAGS ?= -std=c99 -O2 -g -Wall -Wextra -Werror -Wconversion -Wno-sign-conversion
SAN ?= -fsanitize=address,undefined -fno-omit-frame-pointer

SRC = src/cm_core.c src/cm_sha256.c
build/test_core: $(SRC) test/test_core.c include/cm.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SAN) -Iinclude $(SRC) test/test_core.c -o $@

.PHONY: test clean
test: build/test_core
	./build/test_core

clean:
	rm -rf build
