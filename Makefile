TARGET = sandbox

SRC_DIR = src
INC_DIR = include

UNAME_S := $(shell uname -s)

CFLAGS = -Wall -Wextra -Werror -std=c11 -I$(INC_DIR)
LDFLAGS =
LDLIBS = -lncurses -lcjson

ifeq ($(UNAME_S),Darwin)
  PLATFORM = macos
  CC = clang
  CJSON_PREFIX := $(shell brew --prefix cjson 2>/dev/null)
  ifneq ($(MAKECMDGOALS),clean)
    ifeq ($(wildcard $(CJSON_PREFIX)/include/cjson/cJSON.h),)
      $(error cJSON not found. Install it with: brew install cjson)
    endif
  endif
  CFLAGS += -I$(CJSON_PREFIX)/include
  LDFLAGS += -L$(CJSON_PREFIX)/lib
else ifeq ($(UNAME_S),Linux)
  PLATFORM = linux
  CC = gcc
else
  $(error Unsupported platform: $(UNAME_S))
endif

SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/platform/$(PLATFORM)/*.c)
HEADERS = $(wildcard $(INC_DIR)/*.h)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
