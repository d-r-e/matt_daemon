NAME = Matt_daemon

SRC = src/main.cpp src/Daemon.cpp src/TintinReporter.cpp
HEADER = src/main.hpp src/Daemon.hpp src/TintinReporter.hpp 
OBJ = $(SRC:.cpp=.o)

CXX = clang++

CXXFLAGS = -Wall -Wextra -Werror -O2 -g3 -std=c++17

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp $(HEADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

x: all
	./$(NAME)

kill:
	kill -9 `ps aux | grep $(NAME) | grep -v grep | awk '{print $$2}'`

.PHONY: all clean fclean re