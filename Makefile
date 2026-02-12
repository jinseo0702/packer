CC = gcc
RM = rm -rf

NAME = woody_woodpacker

DEBUG ?= 0

CFLAGS = -Wall -Wextra -Werror
CFLAGS += -Wno-unused-parameter
CFLAGS += -Iinclude -Ilibft -Ift_printf
ifeq ($(DEBUG),1)
CFLAGS += -g -DDEBUG
endif

LIBFT_A = libft/libft.a
PRINTF_A = ft_printf/libftprintf.a

SRC = src/woody-packer.c \
	src/error.c \
	src/arg.c \
	src/io_unit.c \
	src/format_router.c \
	src/elf_parser.c \
	src/check_meta.c \
	src/encryption_xor.c \
	src/shellcode_data.c \
	src/enter_data.c \
	src/write_output.c

OBJS = $(SRC:.c=.o)

HEADERS = include/woody.h libft/libft.h ft_printf/libftprintf.h

all: $(NAME)

$(NAME): $(OBJS) $(LIBFT_A) $(PRINTF_A)
	$(CC) -o $@ $(OBJS) $(LIBFT_A) $(PRINTF_A)

$(LIBFT_A):
	make -C libft

$(PRINTF_A):
	make -C ft_printf

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	make clean -C libft
	make clean -C ft_printf
	$(RM) $(OBJS)

fclean: clean
	make fclean -C libft
	make fclean -C ft_printf
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
