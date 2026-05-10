CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Find all .cpp files in the src folder
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
# Create the corresponding .o filenames in the obj folder
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

TARGET = myProg

all: $(TARGET)

# Create the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile object files individually
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create the obj folder if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

run-batch: $(TARGET)
	./$(TARGET) -b data/ranges.txt data/registers.txt data/allocation.txt

.PHONY: all clean run-batch