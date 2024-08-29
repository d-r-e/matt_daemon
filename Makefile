## conventional makefile for binary matt_daemon with all, name, re, fclean, clean rules

NAME = matt_daemon

SRC = main.cpp

OBJ = $(SRC:.cpp=.o)

CXX = clang++

CXXFLAGS = -Wall -Wextra -Werror -O2 -std=c++98

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
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