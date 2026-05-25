obj-m := penguin_fat16.o
penguin_fat16-y := main.o super.o inode.o dir.o file.o fat16.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean
