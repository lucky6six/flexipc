# GDB 调试程序方法

1. 打开两个 terminal：term1和term2；
2. 编译；
3. term1进入chcore代码根目录，运行 `make qemu-gdb`；
4. 在term2中chcore代码根目录，运行 `make gdb`。


对于动态链接的程序：

5. 在term2中调用 `prepare-load-lib`；
6. 在term1中运行动态链接的app；
7. 如果一切正常，term2 中的 GDB 会自动停下来，此时可以设置断点之类的；
8. 后面就按照正常 GDB 进行使用就可以了。

对于静态程序：

5. 在term2中使用 `add-symbol-file-off` 添加目标程序的符号，例如 `add-symbol-file-off user/build/ramdisk/perf_ipc.bin`；
6. 在term2添加断点：`hb main`，并继续 `continue`；
7. 在term1中执行对应程序；
8. 不出意外 GDB 会自动停下来，后面按照正常使用就可以了。


