# IPADS OS microkernel

## Env

gcc for aarch64 is required (version 5.4 is tested)

(aarch64-linux-gnu-gcc v7.4.0 is tested & aarch64-linux-gnu-ld v2.30 is tested)

qemu-4.1 is tested; >= qemu-3.1

fastboot

## For QEMU-Raspi3

### Compile

We suggest to build the project with the provided docker enviroment.


For full-build:
```
make build
```
or

```
./docker_build.sh aarch64 full (default)
./docker_build.sh x86_64 full
```

If you have done full-build and do not further modify libc, you can use fast-build.

For fast-build (do not compile libc):
```
make fastbuild
```
or
```
./docker_build.sh aarch64 fast
./docker_build.sh x86_64 fast
```

Check the usage:
```
./docker_build.sh help
```


### Run
```
make qemu
```
or
```
cd build && ./simulate.sh
```

### Debugging

Please refer to [this document](documents/gdb.md)


## In-system Test Cases

```
./scripts/chcore_sys_tests.exp
```

## For Hikey970

Turn the switch 1 & 3 to ON state and switch 2 & 4 to OFF state

```
./docker_build.sh hikey full
./docker_build.sh hikey fast
```

For the first time, download [Lebian](http://www.lemaker.org/product-hikey970-download-85.html) from 96board and run

```
sudo fastboot flash boot boot-hikey970.uefi.img
```

Then turn the switch 3 to OFF and find a type-c cable and connect to the DEBUG-UART.

Press the RESET button and you are ready to go.

## For shared Hikey970

build the image rootfs.img

```
rm -rf build
sh ./docker_fastbuild.sh -DCHCORE_PLAT=hikey970
cd tools/hikey970
sudo sh ./build_rootfs.sh
```

scp the rootfs.img to the machine where the shared Hikey970 resides

```
scp rootfs.img hikey@chcore-pi:~
```

ssh to the machine where the shared Hikey970 resides

```
ssh hikey@chcore-pi
```

run

```
./flash.sh
sudo minicom
```

## Facebook infer

Infer v0.17.0 is the latest release (2019.10.8)

### Get Infer

#### On Mac

```
brew install infer
```

#### On Linux

```
VERSION=0.17.0; \
curl -sSL "https://github.com/facebook/infer/releases/download/v$VERSION/infer-linux64-v$VERSION.tar.xz" \
| sudo tar -C /opt -xJ && \
ln -s "/opt/infer-linux64-v$VERSION/bin/infer" /usr/local/bin/infer
```

### Use infer

```
mkdir build && cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DFBINFER=1 ..
infer capture --compilation-database compile_commands.json
infer analyze
```

## Tests

### Unit Tests

see `tests/README.md`

### Kernel Tests

To enable kernel tests, using the following commend to compile the kernel:
```
./docker_build.sh xxx xxx tests
```

## Coding Style

Requirements: 

1. Please use #pragma once (instead of #ifndef) in header files.
2. Put all header files in kernel/include directory and, especially, 
   put the architecture-dependent ones in kernel/include/arch/xxx/arch/
   (e.g., kernel/include/arch/aarch64/arch/) directory.

Please use `tools/checkpatch/scripts/checkpatch.pl` to check the style of your source code before committing.

The checkpatch is a submodule and you need to pull it via
```
git submodule init
git submodule update
```

and use it as follows:

```
./tools/checkpatch/scripts/checkpatch.pl -f foo.c
```

[Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html)

[MISRA C](https://www.misra.org.uk/Activities/MISRAC/tabid/160/Default.aspx)

## Gitlab CI

We only enable basic unit test and fbinfer when issuing a new merge request.

When you need to merge into the master, you have to trigger the full tests manually.
1. Go to `CI/CD -> Pipelines`.
2. Click `Run Pipeline`.
3. Select the branch you want to merge.
4. Click the `Run Pipeline` button`.

The CI machine is named yyf@ci.
If gitlab CI crashes, you can log in that machine and execute `pkill gitlab-runner`

## Drivers to get `/dev/ttyXRUSB0`:

```bash
git clone https://github.com/kasbert/epsolar-tracer.git
cd epsolar-tracer/xr_usb_serial_common-1a
make
sudo rmmod cdc_acm
sudo insmod ./xr_usb_serial_common.ko
```

## Misc. debug options / macros

- `TRACE_SYSCALL` (disabled by default) in `user/musl-1.1.24/src/chcore-port/syscall_dispatcher.c` controls whether to print all syscall invocations in libc.
    Not all syscalls are printed. Exceptions include:
    1. SYS_sched_yield (generated too frequently);
    2. SYS_writev, SYS_ioctl and SYS_read (used in printf);
    3. SYS_io_submit (used in printf?).

- `kernel/include/common/debug.h` defines the kernel debugging options which can ease the procedure of bug hunting.

