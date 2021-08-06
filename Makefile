SRC_DIR := .
OBJ_DIR := obj

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

EXEC := yabfi
EXEC_DBG := yabfi_debug


CFLAGS := -std=c99 -Wall -Wextra


.PHONY: all debug clean

all: CFLAGS += -O2
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o$(EXEC) $^

debug: CFLAGS += -g -O0
debug: $(EXEC_DBG)

$(EXEC_DBG): $(OBJ)
	$(CC) -o$(EXEC_DBG) $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< -o$@ $(CFLAGS)

clean:
	@rm -rf $(OBJ_DIR) $(EXEC) $(EXEC_DBG)
