CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic
INCLUDES = -Isrc -Itests

ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0
else
    CFLAGS += -O2
endif

ifeq ($(SANITIZE), 1)
    CFLAGS += -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
endif

ifeq ($(OS),Windows_NT)
    EXE_EXT ?= .exe
else
    EXE_EXT ?=
endif

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
BIN_DIR = bin

CORE_SRCS = $(SRC_DIR)/fact.c $(SRC_DIR)/rule.c $(SRC_DIR)/engine.c $(SRC_DIR)/io.c
CORE_OBJS = $(BUILD_DIR)/fact.o $(BUILD_DIR)/rule.o $(BUILD_DIR)/engine.o $(BUILD_DIR)/io.o
LIB_CORE = $(BUILD_DIR)/libsyllogist_core.a

MAIN_SRC = $(SRC_DIR)/main.c
SYLLOGIST_BIN = syllogist$(EXE_EXT)

TEST_FACT_SRC = $(TEST_DIR)/test_fact.c
TEST_FACT_BIN = $(BIN_DIR)/test_fact$(EXE_EXT)

TEST_RULE_SRC = $(TEST_DIR)/test_rule.c
TEST_RULE_BIN = $(BIN_DIR)/test_rule$(EXE_EXT)

TEST_ENGINE_SRC = $(TEST_DIR)/test_engine.c
TEST_ENGINE_BIN = $(BIN_DIR)/test_engine$(EXE_EXT)

.PHONY: all test clean dirs

all: dirs $(LIB_CORE) $(SYLLOGIST_BIN) $(TEST_FACT_BIN) $(TEST_RULE_BIN) $(TEST_ENGINE_BIN)

dirs:
ifeq ($(OS),Windows_NT)
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
else
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
endif

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIB_CORE): $(CORE_OBJS)
	$(AR) rcs $@ $^

$(SYLLOGIST_BIN): $(MAIN_SRC) $(LIB_CORE) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_CORE) -o $@ $(LDFLAGS)

$(TEST_FACT_BIN): $(TEST_FACT_SRC) $(LIB_CORE) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_CORE) -o $@ $(LDFLAGS)

$(TEST_RULE_BIN): $(TEST_RULE_SRC) $(LIB_CORE) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_CORE) -o $@ $(LDFLAGS)

$(TEST_ENGINE_BIN): $(TEST_ENGINE_SRC) $(LIB_CORE) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_CORE) -o $@ $(LDFLAGS)

test: $(TEST_FACT_BIN) $(TEST_RULE_BIN) $(TEST_ENGINE_BIN)
	@echo "Running unit tests..."
	@$(TEST_FACT_BIN)
	@$(TEST_RULE_BIN)
	@$(TEST_ENGINE_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(SYLLOGIST_BIN)


