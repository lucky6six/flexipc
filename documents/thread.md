# IPADS OS Thread

## Init Thread 与 IDLE Thread

kernel在初始化时会为每个核建立一个最低优先级的kernel thread：Idle thread。该thread会在ready queue为空时选择该优先级最低的thread进行调度。

注意，由于kernel thread的栈指针特殊性，现在的idle thread不允许在里面碰栈指针。如果需要更加复杂的kernel thread，需要新设计kernel thread的现场保护与恢复流程。

另外，kernel会建立一个init thread，赋予该thread全部的cap，并设置其为最高优先级。kernel会利用elfloader从固定symbol处加载放在user目录下一起打包到kernel image的user程序。

## Scheduler: Context Switch

每个user-level thread都有一个kernel中使用的栈。

该栈栈顶保存着这个thread与scheduler相关的context。

thread中的`thread_ctx`就指向该thread的kernel栈的栈顶。

在触发exception时，irq handler会将寄存器现场压入该栈。

而当切换thread或者从irq返回时，只需要从对应thread的该kernel栈顶pop出相应现场，即可通过eret返回到该线程。

## Capability

