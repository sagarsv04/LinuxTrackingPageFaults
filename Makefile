# Make file for building application

CC = gcc
EXTRA_CFLAGS = -Wall -Werror -Wframe-larger-than=4000

KDIR = /lib/modules/$(shell uname -r)/build/
# KDIR = /lib/modules/4.4.0-178-generic/build/

# obj-m += kprobe_example.o
# obj-m += kretprobe_example.o
# obj-m += pf_probe_A.o
obj-m += pf_probe_B.o
# obj-m += pf_probe_C.o

all:
	make -C $(KDIR) M=$(PWD) modules
	$(CC) user.c $(EXTRA_CFLAGS) -o user

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.d user
