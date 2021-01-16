NAME = coord

CFLAGS = -g -Wall -Wextra -Wshadow -Wno-unused-parameter \
	 -Wmissing-prototypes -Wmissing-declarations \
	 -I../libcommon
override CFLAGS += $(CFLAGS_OVERRIDE)
LDLIBS = -L../$(LIB_ARCH_PREFIX)libcommon -lcommon -lpthread -lmosquitto

OBJS = $(NAME).o fsm.o led.o mqtt.o

include Makefile.c-common


.PHONY:		all spotless

all::           $(NAME)

arm:
		$(MAKE) CC=arm-linux-cc LIB_ARCH_PREFIX=arm/ all

$(NAME):	$(OBJS)

spotless::
		rm -f $(NAME)

