NAME        = Gomoku
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++17
SFML_FLAGS  = -lsfml-graphics -lsfml-window -lsfml-system

SRCS        = main.cpp AI.cpp Board.cpp GomokuGame.cpp Zobrist.cpp
OBJS        = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME) $(SFML_FLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
