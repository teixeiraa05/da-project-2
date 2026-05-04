CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Encontra todos os ficheiros .cpp na pasta src/
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
# Cria os nomes dos ficheiros .o equivalentes na pasta obj/
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

TARGET = myProg

all: $(TARGET)

# Cria o executável
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compila os objetos individualmente
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Cria a pasta obj se não existir
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

run-batch: $(TARGET)
	./$(TARGET) -b data/ranges.txt data/registers.txt data/allocation.txt

.PHONY: all clean run-batch