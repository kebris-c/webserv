# OWNER: both (kebris-c sets compiler/flags/link; kmarrero adds new src files when modules land)
# GOAL: Build ./webserv with C++98, -Wall -Wextra -Werror, no unnecessary relinking.
# LEARN: Make pattern rules, dependency tracking, phony targets.

NAME		= webserv

# Subject says compile with c++; on some images `c++` is clang without libstdc++.
# Prefer g++ for C++98 + libstdc++; override with: make CXX=c++
CXX			= g++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
CPPFLAGS	= -Iinclude

SRC_DIR		= src
OBJ_DIR		= obj

SRCS		= \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/Config.cpp \
	$(SRC_DIR)/Server.cpp \
	$(SRC_DIR)/Socket.cpp \
	$(SRC_DIR)/Connection.cpp \
	$(SRC_DIR)/Request.cpp \
	$(SRC_DIR)/Response.cpp \
	$(SRC_DIR)/Router.cpp \
	$(SRC_DIR)/HttpHandler.cpp \
	$(SRC_DIR)/CgiProcess.cpp \
	$(SRC_DIR)/Utils.cpp

OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
