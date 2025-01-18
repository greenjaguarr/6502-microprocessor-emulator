# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g

CXXFLAGS += -I$(BUILD_DIR)


# Directories
SRC_DIR = src
BUILD_DIR = build
ASM_DIR = src/asm_files

# Source files and executable
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
EXEC = $(BUILD_DIR)/main

# Assembler command and files
ASM_FILES = $(ASM_DIR)/fibonachi_numbers.s  # Only the desired .s file
BIN_FILE = $(BUILD_DIR)/memory.bin  # Output of the assembler

# Targets
all: $(EXEC)

# Assemble the .s file into memory.bin in the build directory
$(BIN_FILE): $(ASM_FILES)
	@mkdir -p $(BUILD_DIR)
	vasm6502_oldstyle -Fbin -dotdir -o $(BIN_FILE) $(ASM_FILES)

# Generate the C header file from the binary
build/memory_bin.h: $(BIN_FILE)
	xxd -i $(BIN_FILE) > build/memory_bin.h

# Compile the C++ source files and link them
$(EXEC): $(OBJ_FILES) build/memory_bin.h  # Ensure the header is built first
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile each C++ file into an object file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp build/memory_bin.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the program and show the output
run: $(EXEC)
	./$(EXEC)  # Run the compiled program
	hexdump -C $(BIN_FILE)  # Show the content of the assembled memory.bin

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_FILE) build/memory_bin.h output.bin
