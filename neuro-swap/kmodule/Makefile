# Module name (the .ko will be nswap.ko)
obj-m := nswap.o
nswap-y := kmain.o inference.o math.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
