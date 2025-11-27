NAME = ft_ping
CC = cc 
CFLAGS = -O2 -Wall -Wextra -Werror -I$(DIR_INCLUDE)
LDFLAGS = -lm -O2

DIR_INCLUDE = include

DIR_SRC = src
FILE_SRC = main.c ping.c 

DIR_OBJ = obj
FILE_OBJ = $(FILE_SRC:.c=.o)
OBJS = $(addprefix $(DIR_OBJ)/, $(FILE_OBJ))

all: $(NAME)


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

.PHONY: all clean fclean re
