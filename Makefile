NAME = webserv.exe

SRCS = webserv.cpp \
	   BindingSocket.cpp \
	   ConnectingSocket.cpp \
	   ListeningSocket.cpp \
	   SimpleServer.cpp \
	   TestServer.cpp \
	   main.cpp

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