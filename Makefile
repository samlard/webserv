NAME = webserv

CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -Iincludes

SRCS = srcs/main.cpp \
       srcs/Server.cpp \
       srcs/Client.cpp \
       srcs/Request.cpp \
       srcs/Response.cpp \
       srcs/Config.cpp \
       srcs/CGI.cpp \
       srcs/Utils.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
