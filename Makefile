include ../shared.mk

SOURCES:=$(wildcard *.c)
CFLAGS+=-Og -ggdb3# -Werror

include ../end.mk

# run: main
	# ./main
