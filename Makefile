obj-m += ofniupc.o

KNAME=$(shell uname -r)

.PHONY: all

all:
	make -C /lib/modules/$(KNAME)/build M=$(PWD) modules

insmod: all rmmod
	sudo insmod ./ofniupc.ko

rmmod:
	sudo rmmod ofniupc || true
