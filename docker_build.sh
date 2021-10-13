#!/bin/bash

release_build="-DCMAKE_BUILD_TYPE=Release"
debug_build="-DCMAKE_BUILD_TYPE=Debug"

build_mode=${release_build}
#build_mode=${debug_build}

# Enable kernel tests
if [[ $3 == *"tests"* ]]; then
	echo "Kernel tests enable!"
	kernel_test="-DCHCORE_KERNEL_TEST=ON"
else
	echo "Kernel tests disable!"
	kernel_test="-DCHCORE_KERNEL_TEST=OFF"
fi

function print_usage() {
	RED='\033[0;31m'
	BLUE='\033[0;34m'
	BOLD='\033[1m'
	NONE='\033[0m'

	echo -e "\n${RED}Usage${NONE}:
	.${BOLD}/docker_build.sh${NONE} [OPTION] [FULL/FAST]"

	echo -e "\n${RED}OPTIONS${NONE}:
	${BLUE}aarch64${NONE}: build raspi3 for qemu (default)
	${BLUE}x86_64${NONE}:  build x64 binary for qemu/kvm
	${BLUE}hikey${NONE}: build image for hikey board
	"

	echo -e "\n${RED}FULL/FAST${NONE}:
	${BLUE}full${NONE}: full build with LibC (default)
	${BLUE}fast${NONE}: fast build without LibC
	"

	#echo -e "\n${RED}DEBUG${NONE}:
	#${BLUE}release${NONE}: release mode (default)
	#${BLUE}debug${NONE}: debug mode
	#"
}

function flash_hikey() {
	cd ./tools/hikey970
	sudo sh ./build_rootfs.sh
	sudo fastboot flash system ./rootfs.img
}

build_aarch64="-DCHCORE_ARCH=aarch64"
build_x64="-DCHCORE_ARCH=x86_64"
build_hikey="-DCHCORE_ARCH=aarch64 -DCHCORE_PLAT=hikey970"

# no arguments
if [ $# == 0 ]; then
	echo "Default building: aarch64 raspi3 full"
	./scripts/docker_fullbuild.sh ${build_aarch64} ${build_mode} ${kernel_test}
	exit 0
fi

if [[ $1 == *"help"* ]]; then
	print_usage
	exit 0
fi

# build x86_64
if [[ $1 == *"x86"* ]]; then
	if [ $# == 1 ] || [[ $2 == *"full"* ]]; then
		echo "Building: x86_64 full"
		./scripts/docker_fullbuild.sh ${build_x64} ${build_mode} ${kernel_test}
		exit 0
	fi

	if [[ $2 == *"fast"* ]]; then
		echo "Building: x86_64 fast"
		./scripts/docker_fastbuild.sh ${build_x64} ${build_mode} ${kernel_test}
		exit 0
	fi

	print_usage
	exit 1
fi

# build aarch64
if [[ $1 == *"aarch64"* ]]; then
	if [ $# == 1 ] || [[ $2 == *"full"* ]]; then
		echo "Building: aarch64 full"
		./scripts/docker_fullbuild.sh ${build_aarch64} ${build_mode} ${kernel_test}
		exit 0
	fi

	if [[ $2 == *"fast"* ]]; then
		echo "Building: aarch64 fast"
		./scripts/docker_fastbuild.sh ${build_aarch64} ${build_mode} ${kernel_test}
		exit 0
	fi

	print_usage
	exit 1
fi

#build hikey
if [[ $1 == *"hikey"* ]]; then
	if [ $# == 1 ] || [[ $2 == *"full"* ]]; then
		echo "Building: aarch64(hikey970) full"
		./scripts/docker_fullbuild.sh ${build_hikey} ${build_mode} ${kernel_test}
		flash_hikey
		exit 0
	fi

	if [[ $2 == *"fast"* ]]; then
		echo "Building: aarch64(hikey970) fast"
		./scripts/docker_fastbuild.sh ${build_hikey} ${build_mode} ${kernel_test}
		flash_hikey
		exit 0
	fi

    if [[ $2 == *"img"* ]]; then
        echo "Building: hikey970 image full"
		./scripts/docker_fullbuild.sh ${build_hikey} ${build_mode} ${kernel_test}
	    cd ./tools/hikey970
	    sudo sh ./build_rootfs.sh
		exit 0
	fi
	print_usage
	exit 1
fi

print_usage
exit 1

