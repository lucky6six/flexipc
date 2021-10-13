#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# `main $@` is called at the end of the file

main() {

    SCRIPT_ARGS=$@

    TOP_DIR=$(pwd)
    USER_DIR=$TOP_DIR/user

    if [[ $1 == "-fast" ]]; then
        # with "-fast", we skipped libc compilation
        shift
        SCRIPT_ARGS=$@
    else
        build_libc $SCRIPT_ARGS
    fi

    prepare_simulate_sh $SCRIPT_ARGS

    #echo "compiling busybox..."
    #build_busybox $SCRIPT_ARGS

    echo "compiling user directory..."
    build_apps $SCRIPT_ARGS

    echo "compiling ramdisk..."
    build_ramdisk $SCRIPT_ARGS

    echo "compiling kernel..."
    build_kernel $SCRIPT_ARGS

    exit 0
}


############ Helper functions ##############

build_kernel() {
    cd $TOP_DIR/build

    if [[ $@ == *"x86_64"* ]]; then
        cmake -DCMAKE_C_LINK_EXECUTABLE="<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" .. -G Ninja "$@"
    else
        cmake -DCMAKE_LINKER=aarch64-linux-gnu-ld -DCMAKE_C_LINK_EXECUTABLE="<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" .. -G Ninja "$@"
    fi

    # GUI cmake configure
    #ccmake -DCMAKE_LINKER=aarch64-linux-gnu-ld -DCMAKE_C_LINK_EXECUTABLE="<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" .. -G Ninja "$@"

    ninja
    # print the compile logs
    #ninja -v
}

prepare_simulate_sh() {
    cd $TOP_DIR
    if [ ! -d "build" ]; then
        mkdir build
        cd build

		# arch-independent scripts
        cat > simulate.sh <<EOF
#!/bin/bash

basedir=\$(dirname "\$0")

while true; do
	port=\$(shuf -i 30000-40000 -n 1)
	netstat -tan | grep \$port > /dev/null 2>&1
	if [[ \$? -ne 0 ]]; then
		break
	fi
done
echo \$port > \$basedir/gdb-port

EOF

		# arch-specific
        if [[ $@ == *"x86_64"* ]]; then
            cat >> simulate.sh <<EOF
qemu-system-x86_64 -gdb tcp::\$port --enable-kvm -m 4G -cpu host -smp 4 -serial mon:stdio -nographic -kernel \$basedir/kernel.img \$1 | tee exec_log
EOF
        else
            cat >> simulate.sh <<EOF
qemu-system-aarch64 -gdb tcp::\$port -machine raspi3 -nographic -serial null -serial mon:stdio -m size=1G -kernel \$basedir/kernel.img \$1 | tee exec_log
EOF
        fi
        chmod +x simulate.sh
        cd ..
    fi
}

build_libc() {

    set -e
    echo "Full build: compiling libc first ... wait for one minute ..."

    cd $USER_DIR/musl-1.1.24

    rm -rf build
    rm -rf obj
    rm -rf lib

    mkdir build

    if [[ $@ == *"x86"* ]]; then
        echo "compiling: x86_64 libc"
        #./configure --prefix=$PWD/build --syslibdir=$PWD/build/lib --enable-debug  > /dev/null
        ./configure --prefix=$PWD/build --syslibdir=$PWD/build/lib > /dev/null
    else
        echo "compiling: aarch64 libc"
        #./configure --prefix=$PWD/build --syslibdir=$PWD/build/lib \
        #    CROSS_COMPILE=aarch64-linux-gnu- --enable-debug > /dev/null
        ./configure --prefix=$PWD/build --syslibdir=$PWD/build/lib \
            CROSS_COMPILE=aarch64-linux-gnu- > /dev/null
    fi

    make -j4 -s
    make install -s

    echo "Full build: succeed in compiling libc."
}

build_busybox() {

    cd $USER_DIR/busybox-1.31.1

    make clean
    # to add/remove applets from busybox, you can either uncomment the follow `make` or manually edit .config file
    # make menuconfig

    if [[ $@ == *"x86"* ]]; then
        echo "compiling: x86_64 busybox"
        LDFLAGS="--static" make -j
    else
        echo "compiling: aarch64 busybox"
        LDFLAGS="--static" make -j CROSS_COMPILE=$USER_DIR/musl-1.1.24/build/bin/musl-
    fi

    echo "succeed in compiling busybox."
}

build_apps() {

    cd $USER_DIR
    rm -rf build && mkdir build

    C_FLAGS="-O3 -ffreestanding -Wall -Werror -fPIC -static"
    if [[ $@ == *"x86"* ]]; then
        echo "compiling: x86_64 user directory"
        #C_FLAGS="$C_FLAGS -mno-sse -DCONFIG_ARCH_X86_64 -mfsgsbase"
        C_FLAGS="$C_FLAGS -msse -DCONFIG_ARCH_X86_64 -mfsgsbase"
        cd build
        cmake .. -DCMAKE_C_FLAGS="$C_FLAGS" -G Ninja
    else
        echo "compiling: aarch64 user directory"
        C_FLAGS="$C_FLAGS -DCONFIG_ARCH_AARCH64"
        cd build
        cmake .. -DCMAKE_C_FLAGS="$C_FLAGS" -DCHCORE_MARCH="armv8-a" -G Ninja
    fi

    ninja

    echo "succeed in compiling user."
}

build_ramdisk() {
    cd $USER_DIR/build

    echo "building ramdisk ..."

    mkdir -p ramdisk

    #cd $USER_DIR/busybox-1.31.1
    #cp busybox $USER_DIR/build/ramdisk/
    #make clean
    #cd $USER_DIR/build

    cp ../test.txt ramdisk/

    echo "copy musl/libc.so to ramdisk."
    cp ../musl-1.1.24/build/lib/libc.so ramdisk/

    echo "copy dynamic_apps/*.(bin & so) to ramdisk."
    cp dynamic_apps/*.bin ramdisk/
    cp dynamic_apps/*.so ramdisk/

    echo "copy user/*.bin to ramdisk."
    cp apps/*.bin ramdisk/

    echo "copy user/tests/*.bin to ramdisk."
    cp apps/tests/*.bin ramdisk/

    echo "copy user/perf/*.bin to ramdisk."
    cp apps/perf/*.bin ramdisk/

    echo "copy lwip.srv to ramdisk."
    cp lwip/lwip.srv ramdisk/

    echo "copy procmgr.srv to ramdisk."
    cp procmgr/procmgr.srv ramdisk/

    echo "copy tcp_example.bin to ramdisk."
    cp lwip/tcp_example.bin ramdisk/

    echo "copy driver_hub.srv to ramdisk."
    cp drivers/driver_hub.srv ramdisk/

    echo "copy ufs/*srv to ramdisk (only used for aarch64)."
    mkdir -p ramdisk/drivers
    [[ -d drivers/ufs ]] && cp drivers/ufs/*.srv ramdisk/drivers/

    echo "copy dtb to ramdisk (not useful for all arch/plat)"
    cp ../../tools/hikey970/rootfs/boot/kirin970-hikey970.dtb ramdisk/

    mv ramdisk/init.bin .

    cd ramdisk
    find . | cpio -o -Hnewc > ../ramdisk.cpio
    echo "succeed in building ramdisk."

    cd ..
    rm -rf ramdisk-tmp
    mkdir -p ramdisk-tmp
    cd ramdisk-tmp

    cp ../tmpfs/tmpfs.srv .
    echo tmpfs.srv | cpio -o -Hnewc > ../tmpfs.cpio
    echo "Archive tmpfs.srv into the kernel image"

    rm -r *
    cp ../fsm/fsm.srv .
    echo fsm.srv | cpio -o -Hnewc > ../fsm.cpio
    echo "Archive fsm.srv into the kernel image"

    rm -r *
    mv ../init.bin .
    echo init.bin | cpio -o -Hnewc > ../init.cpio
    echo "Archive init.bin into the kernel image"
}


############ Call main ##############
main $@
