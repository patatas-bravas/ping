NAME = ping
CC = cc
CFLAGS = -Wall -Wextra -Werror -O2 -I$(DIR_INCLUDE)
LDFLAGS = -O2

DIR_INCLUDE = include

DIR_SRC = src
FILE_SRC = main.c ping.c utils.c printer.c

DIR_OBJ = obj
FILE_OBJ = $(FILE_SRC:.c=.o)
OBJS = $(addprefix $(DIR_OBJ)/, $(FILE_OBJ))

all: $(NAME)
	./$< www.google.com

build: $(NAME)

$(NAME): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c | $(DIR_OBJ)
	$(CC) $< $(CFLAGS) -c -o $@

$(DIR_OBJ):
	mkdir $@

clean:
	rm -rf $(DIR_OBJ)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all build clean fclean re
