# Make file for building application

CC = gcc
EXTRA_CFLAGS = -Wall -Werror


# obj-m += kprobe_example.o
# obj-m += kretprobe_example.o
obj-m += pf_probe_A.o
# obj-m += pf_probe_B.o
# obj-m += pf_probe_C.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	# $(CC) user.c $(EXTRA_CFLAGS) -o user

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -f *.o *.d
