# ══════════════════════════════════════════════════════════════════
#  AntiGravity Chatbot v2 — Makefile
#  Targets:  all, test, clean
#  Compiler: gcc (MinGW on Windows, GCC on Linux/macOS)
# ══════════════════════════════════════════════════════════════════

CC      = gcc
CFLAGS  = -std=c11 -O2 -Wall -Wextra -Wpedantic -Idata
LIBS    = -lm

ifeq ($(OS),Windows_NT)
  LIBS     += -lws2_32
  EXT       = .exe
  RM        = del /Q /F 2>NUL
  RMDIR     = rmdir /S /Q 2>NUL
else
  LIBS     += -lpthread
  EXT       =
  RM        = rm -f
  RMDIR     = rm -rf
endif

# ── Source files ───────────────────────────────────────────────────
SRC_DIR  = src
MEM_DIR  = src/memory
TEST_DIR = tests

SRCS = $(SRC_DIR)/utils.c       \
       $(SRC_DIR)/tokenizer.c   \
       $(SRC_DIR)/sentiment.c   \
       $(SRC_DIR)/antigravity.c \
       $(SRC_DIR)/memory.c      \
       $(SRC_DIR)/model.c       \
       $(SRC_DIR)/plugins.c     \
       $(SRC_DIR)/server.c      \
       $(SRC_DIR)/embed.c       \
       $(MEM_DIR)/semantic.c    \
       $(MEM_DIR)/profile.c     \
       $(SRC_DIR)/main.c

# Shared library objects (everything except main)
LIB_SRCS = $(filter-out $(SRC_DIR)/main.c, $(SRCS))

TARGET   = chatbot$(EXT)

# Test binaries
TESTS = test_tokenizer$(EXT) \
        test_memory$(EXT)    \
        test_sentiment$(EXT) \
        test_antigravity$(EXT)\
        test_semantic$(EXT)  \
        test_profile$(EXT)   \
        test_runner$(EXT)

# ── Targets ────────────────────────────────────────────────────────
.PHONY: all test clean help

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)
	@echo "Build complete: $(TARGET)"

# Each test standalone binary
test_%$(EXT): $(TEST_DIR)/test_%.c $(LIB_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test_runner$(EXT): $(TEST_DIR)/test_runner.c
	$(CC) $(CFLAGS) -o $@ $^

test: test_tokenizer$(EXT) test_memory$(EXT) test_sentiment$(EXT) test_antigravity$(EXT) test_semantic$(EXT) test_profile$(EXT) test_runner$(EXT)
	./test_runner$(EXT)

clean:
	$(RM) $(TARGET) $(TESTS) data/*.bin 2>NUL || true
	@echo "Clean complete."

help:
	@echo "Targets:"
	@echo "  make          Build chatbot$(EXT)"
	@echo "  make test     Build and run all unit tests"
	@echo "  make clean    Remove build artefacts"
