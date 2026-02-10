CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic
TEST = test_parser
EXAMPLE = example_usage

PARSER_SRCS = HttpRequest.cpp
PARSER_OBJS = $(PARSER_SRCS:.cpp=.o)

TEST_SRCS = test_parser.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

EXAMPLE_SRCS = example_usage.cpp
EXAMPLE_OBJS = $(EXAMPLE_SRCS:.cpp=.o)

all: $(TEST) $(EXAMPLE)

$(TEST): $(PARSER_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TEST) $(PARSER_OBJS) $(TEST_OBJS)

$(EXAMPLE): $(PARSER_OBJS) $(EXAMPLE_OBJS)
	$(CXX) $(CXXFLAGS) -o $(EXAMPLE) $(PARSER_OBJS) $(EXAMPLE_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(PARSER_OBJS) $(TEST_OBJS) $(EXAMPLE_OBJS)

fclean: clean
	rm -f $(TEST) $(EXAMPLE)

re: fclean all

.PHONY: all clean fclean re
