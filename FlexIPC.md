# **Exception-less Inter-Process Communication**

## 1. Overview
FlexSC provides an asynchronous mechanism to support syscalls in Linux. The syscall in ChCore is different to that in Linux, as the syscall in ChCore may be converted to an IPC to a corresponding server. For example, a `read` syscall to a socket is converted to an IPC to LWIP server. In this lab, you are supposed to implement **FlexIPC** (similar to FlexSC but aims at IPCs) to optimize the IPC to LWIP server. This lab consists of four steps.


In step A, you should read [FlexSC](https://www.usenix.org/legacy/events/osdi10/tech/full_papers/Soares.pdf). When you read this paper, it is better to think about how to implement all the techniques mentioned in the paper.

In step B, you will modify musl libc and LWIP's IPC handling procedure to handle IPC invocations without context switches.

In step C, you will implement a user-space thread scheduling framework.

In step D, you will create several threads in clients and LWIP server to provide better concurrency.




## 2. Step B: FlexIPC with Single Producer and Consumer

Firstly , you should learn how syscalls related to socket are converted to IPCs in musl libc (`user/musl/src/chcore/syscall_dispather.c, fd.c, socket.c`) and how LWIP handles IPCs issued by clients (`user/lwip/main.c`). 

You are also required to get familiar with how IPC is implemented (`user/musl/src/chcore/ipc.c` and `kernel/ipc/connection.c`). ChCore's IPC facility provides shared memory between the client and server. A client invokes an IPC by:

1. Client gets a memory region from shared memory by `ipc_create_msg`;
2. Client sets up arguments in the shared memory region by `ipc_set_msg_data`;
3. Client invoke `ipc_call` to switch to server's IPC handler;
4. Server's IPC handler gets arguments from shared memory by `ipc_set_msg_data`;
5. After handling the IPC, server returns to client by `ipc_return` ;
6. Client frees the shared memory region by `ipc_destroy_msg`.

You are supposed to change step 3 and 5 above to support a basic exception-less IPC bwteen client and server (with one client thread and one server handler thread). Now an IPC invocation procedure is modified in the following way:

* Client invokes a new function `ipc_call_flex` in IPC library. Different from existing `ipc_call` which switches to server's context,  `ipc_call_flex` should mark the request in shared memory as **submitted** and check a flag in a dead loop to check whether the servers‘ handling is **done**. 
* You should create a **handler thread** in server which scans the shared memory to check whether there are requests **submitted**. After the server finds a ready request, it handles the IPC request and marks it as **done** in shared memory.

Feel free to manage the memory layout of requests (arguments, flags used to synchronize, etc.) in shared memory.  You should **bind** client and server threads to differenct cores to prevent their interference with each other.

You are required to modify musl libc to use `ipc_call_flex` for socket-related syscalls and modify LWIP server to handle exception-less IPC. You can use `user/apps/tests/test_net_client.c,test_net_server.c` as clients to LWIP server for testing.



## 3. Step C: User-level Scheduling
After finishing the above single producer and consumer model, now we try to support multiple client threads (like coroutines) with **FlexIPC**. Each client thread can issue an IPC request and then switch to another client thread **instead of busy-waiting**. To avoid the involvement of kernel, we should use user-level scheduling instead of conventional multithread support provided by kernel. More specifically, you should:

1. Design how to store multiple IPC requests in shared memory. Extend `ipc_create_msg` to allocate shared memory regions for multiple threads without overlapping.

2. Maintain a **wait queue** and a **ready queue** which record threads waiting for IPC results or not, individually. When a thread invokes IPC by `ipc_call_flex`,  it should be put into the **wait queue** and schedule to a thread in the **ready queue**. If the **ready queue** is empty, check if any IPC request is **done** and move the corresponding threads from the **wait queue** to the **ready queue**.

3. The LWIP should also support user-level threads. These threads should concurrently scan shared memory to handle each request in shared memory sequentially.


## 4. Evaluation

After finishing all the four steps, you can evaluate the read throughput of`test_net_client.c` and compare its performance with the old one without FlexIPC support. Further, you need to write a simple benchmark to measure the throughput **send** operations which are non-blocking operations. You may find that non-blocking operations can benefit more from FlexIPC. You should explain the reasons for the performance improvement by using experiment data.  Please note that you can modify the test programs in order to support user-level scheduling and some network operations.


## 5. Advice and Further Requirements

### ADVICE
* **About implementing**: Try to divide your implementation process into multiple small steps. When you finish one small step, recompile the code and test it before advancing to the next step.
* **About debugging**: you may encounter many obscure bugs when implementing FlexIPC. Adding print and debugging using virtual machines are the most common techniques. It is a good opportunity for you to get familiar with these techniques.
* **Benchmark programs**: you can modify the test programs in order to support user-level scheduling.

### Requirements
* Please do not publish any notes about this lab on the internet or share them with others. We will keep using this lab to train future IPADS newcomers. 
* Please do not share your code with other trainees. 

