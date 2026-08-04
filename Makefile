# ==============================================================================
# Makefile - ATM Simulasyonu
# ==============================================================================
# Kullanim:
#   make            -> Projeyi derler (bin/atm_sim uretir)
#   make run        -> Derler ve programi calistirir
#   make debug      -> Debug sembolleri (-g) ve AddressSanitizer ile derler
#   make clean      -> Derleme ciktilarini temizler
#   make clean-data -> Kalici veri dosyalarini (data/) da temizler (DIKKAT!)
# ==============================================================================

CC       := gcc
CSTD     := -std=c11
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion
OPT      := -O2

CFLAGS   := $(CSTD) $(WARNINGS) $(OPT)
LDFLAGS  :=

SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build
BIN_DIR   := bin

TARGET := $(BIN_DIR)/atm_sim

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))
DEPS    := $(OBJECTS:.o=.d)

.PHONY: all run debug clean clean-data directories

all: directories $(TARGET)

directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) data

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo ""
	@echo "Derleme basarili -> $(TARGET)"

# Header bagimliliklarini otomatik takip et (.d dosyalari ile)
-include $(DEPS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | directories
	$(CC) $(CFLAGS) -I$(INC_DIR) -MMD -MP -c $< -o $@

run: all
	@./$(TARGET)

debug: CFLAGS += -g -O0 -fsanitize=address,undefined
debug: LDFLAGS += -fsanitize=address,undefined
debug: clean all

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

clean-data:
	rm -rf data/*.dat data/*.log
