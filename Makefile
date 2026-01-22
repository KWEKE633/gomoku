# NAME        = Gomoku
# CXX         = c++
# CXXFLAGS    = -Wall -Wextra -Werror -std=c++17
# SFML_FLAGS  = -lsfml-graphics -lsfml-window -lsfml-system

# SRCS        = main.cpp
# OBJS        = $(SRCS:.cpp=.o)

# all: $(NAME)

# $(NAME): $(OBJS)
# 	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME) $(SFML_FLAGS)

# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# clean:
# 	rm -f $(OBJS)

# fclean: clean
# 	rm -f $(NAME)

# re: fclean all

# .PHONY: all clean fclean re




# mac版

NAME        = Gomoku


CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++17

# MacでのSFMLのパス（Homebrewで入れた場合の標準的なパス）
# もし場所が違う場合はここを調整します
SFML_PATH   = /opt/homebrew
SFML_FLAGS  = -L$(SFML_PATH)/lib -lsfml-graphics -lsfml-window -lsfml-system
INC         = -I$(SFML_PATH)/include

SRCS        = main.cpp
OBJS        = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME) $(SFML_FLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
