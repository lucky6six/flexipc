BUILD_DIR := ./build
ifndef QEMU
QEMU := qemu-system-aarch64
endif

all: build

gdb:
	gdb-multiarch -n -x .gdbinit

build:
	./docker_build.sh aarch64 full

fastbuild:
	./docker_build.sh aarch64 fast

qemu: $(IMAGES) 
	@echo "***"
	@echo "*** Use Ctrl-a x to exit qemu"
	@echo "***"
	@cd build; ./simulate.sh

qemu-gdb: $(IMAGES) 
	@echo "***"
	@echo "*** Use Ctrl-a x to exit qemu"
	@echo "***"
	@cd build; ./simulate.sh -S

print-qemu:
	@echo $(QEMU)

print-gdbport:
	@echo $(GDBPORT)

docker:	
	./scripts/run_docker.sh

.PHONY: clean
clean:
	@rm -rf build
