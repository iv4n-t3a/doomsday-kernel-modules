BATS        := ../contrib/bats-core/bin/bats
KERNEL_HDR  := /lib/modules/$(shell uname -r)/build
GEN_CMDS    := $(KERNEL_HDR)/scripts/clang-tools/gen_compile_commands.py

.PHONY: all test compile_commands clean

all:
	$(MAKE) -C kmodule

test: all
	sudo $(BATS) utests/fat16.bats

compile_commands: all
	python3 $(GEN_CMDS) -d kmodule -o kmodule/compile_commands.json kmodule

clean:
	$(MAKE) -C kmodule clean
	rm -f kmodule/compile_commands.json
