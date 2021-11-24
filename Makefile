SHELL = bash

CFLAGS := \
	-std=c11 \
	-O2 \
	-fanalyzer \
	-Wall \
	-Wextra \
	-Wno-unused-function \
	-Wno-sign-compare \
	-Wno-implicit-fallthrough \
	-Wno-switch

CPPFLAGS := -MD -MP

LDFLAGS := -pthread
LDLIBS :=

TARGET := src/cli.c src/server.c
TARGET_BIN := $(patsubst src/%.c,bin/%,$(TARGET))
TARGET_OBJ := $(patsubst %.c,build/%.o,$(TARGET))

SRC := $(filter-out $(TARGET),$(wildcard src/*.c))
SRC_OBJ := $(patsubst %.c,build/%.o,$(SRC))

TEST := $(wildcard test/*.c)
TEST_OBJ := $(patsubst %.c,build/%.o,$(TEST))
TEST_BIN := $(patsubst %.c,bin/%,$(TEST))

BIN := $(TARGET_BIN) $(TEST_BIN)
OBJ := $(TARGET_OBJ) $(SRC_OBJ) $(TEST_OBJ)

.PHONY: all clean test tree

all: $(BIN) $(OBJ)

clean:
	@rm -rf bin build

test: $(TEST_BIN)
	@for i in $(TEST_BIN); do printf "$$i: "; ./$$i ; done

tree:
	@mkdir -p {build,bin}/{src,test}

$(TARGET_BIN): bin/% : build/src/%.o $(SRC_OBJ)
$(TEST_BIN): bin/% : build/%.o

$(BIN): | tree
	$(CC) $(LDFLAGS) $(LDLIBS) $^ -o $@

$(OBJ): build/%.o : %.c | tree
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(shell find build -name *.d 2>/dev/null)i
