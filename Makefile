NAME	= philo
CC		= cc
CFLAGS	= -Wall -Wextra -Werror -pthread
SRCS	= main.c init.c utils.c lifecycle.c monitor.c banker.c chandy.c
OBJS	= $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c philo.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
