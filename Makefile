NAME = anonymous.exe

SRCS = SocketUtils.cpp \
	   Parse.cpp \
	   main.cpp \
	   Core.cpp \
	   Client.cpp \
	   CgiExecute.cpp \
	   Respond.cpp \
	   Helper.cpp

OBJS = $(SRCS:.cpp=.o)
CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re