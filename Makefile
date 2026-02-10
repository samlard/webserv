NAME = webserv_parser
TEST_NAME = test_parser

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./include
LDFLAGS =

SRC_DIR = src
OBJ_DIR = obj
TEST_DIR = tests
INC_DIR = include

SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/Config.cpp \
       $(SRC_DIR)/ServerConfig.cpp \
       $(SRC_DIR)/RouteConfig.cpp \
       $(SRC_DIR)/Token.cpp \
       $(SRC_DIR)/Tokenizer.cpp \
       $(SRC_DIR)/ConfigParser.cpp

LIB_SRCS = $(SRC_DIR)/Config.cpp \
           $(SRC_DIR)/ServerConfig.cpp \
           $(SRC_DIR)/RouteConfig.cpp \
           $(SRC_DIR)/Token.cpp \
           $(SRC_DIR)/Tokenizer.cpp \
           $(SRC_DIR)/ConfigParser.cpp

TEST_SRCS = $(TEST_DIR)/test_parser.cpp

OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
LIB_OBJS = $(LIB_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.cpp=$(OBJ_DIR)/test_%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_NAME): $(LIB_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(LIB_OBJS) $(TEST_OBJS) -o $(TEST_NAME) $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_NAME)

re: fclean all

test: $(TEST_NAME)
	./$(TEST_NAME)

demo: $(NAME)
	@echo "=== Testing with full configuration ==="
	./$(NAME) examples/config.conf
	@echo ""
	@echo "=== Testing with minimal configuration ==="
	./$(NAME) examples/minimal.conf

.PHONY: all clean fclean re test demo
