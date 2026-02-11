NAME     = anonymous.exe
SRC_DIR  = src
OBJ_DIR  = obj
INC_DIR  = inc

SRCS     = $(SRC_DIR)/SocketUtils.cpp \
           $(SRC_DIR)/Parse.cpp \
           $(SRC_DIR)/main.cpp \
           $(SRC_DIR)/Core.cpp \
           $(SRC_DIR)/Client.cpp \
           $(SRC_DIR)/CgiExecute.cpp \
           $(SRC_DIR)/Respond.cpp \
           $(SRC_DIR)/Helper.cpp

# This correctly changes src/file.cpp to obj/file.o
OBJS     = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I $(INC_DIR)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

cleanly: all clean

.PHONY: all clean fclean re cleanly