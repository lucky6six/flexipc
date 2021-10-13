# Exception Handler (ARM)

目前异常处理分为两部分：

- BOOT时的Exception Handler。该Handler主要用于在开启正式的Handler之前debug使用。其会catch住所有的exception然后将其打印出来。其入口定义于`bootloader/init/irq.S`。
- Kernel中的Exception Handler。



## Kernel中异常处理流程

目前的处理流程是：

在`kernel/arch/xxx/irq/irq_entry.S`定义分发异常。

- 若为`IRQ`，则在`irq_entry.c`中的`handle_irq`交给平台相关的`plat_handle_irq`处理。最后调用scheduler切换到合适的线程。
- 若为`SYSCALL`，则利用`kernel/syscall/syscall.c`中的`syscall_table`通过偏移量找到对应函数的入口。
- 若为其他异常，如Page Fault，则在`irq_entry.c`中的`handle_entry_c`根据EC的不同分发到不同的入口。`handle_entry_c`中传入的两个参数：`esr`为Exception Syndrome Register，包含了导致这个exception的原因，从manual抄下来的远影在`register.h`中；`address`是导致这个异常的地址。



## Timer

### Generic Timer

- 包含per cpu的timer，以及system level的counter。
- 访问per cpu的timer使用CP15来访问，访问system level的counter使用MMIO来访问。
- 每个处理器可以通过CNTPCT寄存器来获取system counter的值。
- 对于支持security extension和virtualization extension的SOC
  - Non-secure PL1 physical timer
  - Secure PL1 physical timer
  - Non-secure PL2 physical timer
  - virtual timer：A virtual counter that indicates virtual time. The virtual counter contains the value of the physical counter minus a 64-bit virtual offset.
- 每个timer有三个寄存器

  - 64-bit CompareValue register
  - 32-bit TimerValue register
  - 32-bit control register


### Ref

Chapter D10 in ARMARM
