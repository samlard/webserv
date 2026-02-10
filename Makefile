NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic -g

SRCS = src/main.cpp \
       src/Server.cpp \
       src/HTTPRequest.cpp \
       src/HTTPResponse.cpp \
       src/CGIHandler.cpp \
       src/Utils.cpp

OBJS = $(SRCS:.cpp=.o)

INC = -I include

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
