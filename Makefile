NAME = ping
CC = cc
C_FLAGS = -Wall -Wextra -Werror -g

DIR_INCLUDE = include

DIR_SRC = src
FILE_SRC = main.c
SRCS = $(addprefix $(DIR_SRC)/, $(FILE_SRC))

DIR_OBJ = obj
FILE_OBJ = $(FILE_SRC:.c=.o)
OBJS = $(addprefix $(DIR_OBJ)/, $(FILE_OBJ))

 
all: $(NAME)
	./$<


$(NAME): $(OBJS)
	$(CC) $< -I$(DIR_INCLUDE) -o $@

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c | $(DIR_OBJ)
	$(CC) $< -c -o $@

$(DIR_OBJ):
	mkdir $@

clean:
	rm -rf $(DIR_OBJ)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re



