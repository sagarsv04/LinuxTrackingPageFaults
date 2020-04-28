# Make file for building application

CC = gcc
EXTRA_CFLAGS = -Wall -Werror


obj-m += dev_ps.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	$(CC) user.c $(EXTRA_CFLAGS) -o user

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -f *.o *.d user
