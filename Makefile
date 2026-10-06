NAME		= codexion

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread -Iincludes

SRCS_DIR	= srcs
OBJS_DIR	= objs

SRCS		= $(SRCS_DIR)/main.c \
			  $(SRCS_DIR)/parsing.c \
			  $(SRCS_DIR)/time_utils.c \
			  $(SRCS_DIR)/pqueue.c \
			  $(SRCS_DIR)/pqueue_ops.c \
			  $(SRCS_DIR)/dongle.c \
			  $(SRCS_DIR)/coder.c \
			  $(SRCS_DIR)/monitor.c \
			  $(SRCS_DIR)/cleanup.c

OBJS		= $(SRCS:$(SRCS_DIR)/%.c=$(OBJS_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c includes/codexion.h | $(OBJS_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
