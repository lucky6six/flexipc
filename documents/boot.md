# *IPADS OS* Boot、

[TOC]

## ARCH-aarch64

不同于seL4，目前的bootloader直接和kernel的obj放在一个elf里面，而seL4则将kernel image和dts以CPIO形式在bootloader中加载（move到指定物理地址）。
Bootloader目前的职责是初始化processor、填页表、读取device tree。
详细的，Bootloader主要完成以下内容：

1. Hang住除了primary cpu之外的CPU，等待在一个per CPU的标识位。
2. 从EL3切到EL1
3. [TODO] Load Device Tree 
4. 填页表并enable MMU
5. 跳到kernel指定entry

虚拟地址空间映射如下：[TODO]

针对SMP的支持：在Primary CPU已经完成初始化后，通过修改其余CPU所等待的标识位允许其他CPU初始化。
其余CPU初始化的流程与Primary CPU类似。主要完成上述的2，4，5步。



### Platform-Raspi3

目前Raspi3用qemu起来使用的是直接`-kernel`参数后指定`elf`来执行。`elf`指定了entry point。

[TODO] 使用qemu起UEFI，保持与Hikey970的一致。



### Platform-Hikey970

Hikey970上起OS必须要走efi。efi主要是需要一个header以及指定`efi_stub_entry`。

从UEFI拿到控制权时，为el2特权级，且页表已开。

在上述通用的启动之前，bootloader还执行了以下步骤（`efi_stub_entry`）：

1. 获取physical memory map，后面传给kernel
2. 安排UEFI的run-time services在kernel开了页表之后的虚拟地址
3. 通知UEFI更新后的虚拟地址
4. exit boot
5. 关闭页表

#### UART

The HiKey board also has an option for a Debug UART Type C J3101. This is normally used by the first stage bootloader developers, and is connected to the UART6 port of the SoC. 



### Refs

https://www.linaro.org/blog/u-boot-on-arm32-aarch64-and-beyond/
https://balau82.wordpress.com/2010/04/12/booting-linux-with-u-boot-on-qemu-arm/

Linux Documentation/efi-stub.txt



## ARCH-x86

