# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: webserv project                           +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/01/01 00:00:00 by webserv          #+#    #+#              #
#    Updated: 2024/01/01 00:00:00 by webserv         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		= webserv
CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -I includes
RM			= rm -f

SRCS		= sources/main.cpp \
			  sources/Config.cpp \
			  sources/Socket.cpp \
			  sources/Server.cpp \
			  sources/Client.cpp \
			  sources/HttpRequest.cpp \
			  sources/HttpResponse.cpp \
			  sources/RequestHandler.cpp \
			  sources/CgiHandler.cpp

OBJS		= $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
