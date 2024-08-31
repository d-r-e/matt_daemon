NAME = Matt_daemon

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

SRC = $(SRC_DIR)/main.cpp $(SRC_DIR)/Daemon.cpp $(SRC_DIR)/TintinReporter.cpp
HEADER = $(INCLUDE_DIR)/main.hpp $(INCLUDE_DIR)/Daemon.hpp $(INCLUDE_DIR)/TintinReporter.hpp
OBJ = $(BUILD_DIR)/main.o $(BUILD_DIR)/Daemon.o $(BUILD_DIR)/TintinReporter.o

CXX = clang++
CXXFLAGS = -Wall -Wextra -Werror -O2 -I $(INCLUDE_DIR) -g3 -std=c++17

all: $(BIN_DIR)/$(NAME)

$(BIN_DIR)/$(NAME): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(INCLUDE_DIR)/main.hpp $(HEADER)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Daemon.o: $(SRC_DIR)/Daemon.cpp $(INCLUDE_DIR)/Daemon.hpp $(HEADER)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/TintinReporter.o: $(SRC_DIR)/TintinReporter.cpp $(INCLUDE_DIR)/TintinReporter.hpp $(HEADER)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -rf $(BIN_DIR)

re: fclean all

x: all
	./$(BIN_DIR)/$(NAME)

kill:
	kill -9 `ps aux | grep $(NAME) | grep -v grep | awk '{print $$2}'`

.PHONY: all clean fclean re
