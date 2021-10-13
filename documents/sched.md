# *IPADS OS* Scheduler Spec

与thread相关，包含上下文切换，保留恢复现场的设计见thread

[TOC]

# Without Real Time Support (Simpler version)

## Workflow

1. scheduler invoked at the end of each interrupt handler
1.1 timer interrupt may change the state of the thread/scheduler
1.2 for other interrupts, scheduler is required (maybe switch to another thread)
1.3 thread can use yield syscall
2. when timer interrupt arrives
2.1 check and update thread's time slice (flag `reschedule_needed`)
2.2 if reschedule (switch) is needed
2.2.1 call `schedule()`
2.2.1.1 enqueue old thread
2.2.1.1 choose & dequeue new thread
2.2.2 call `activate_thread()`

## Data stuctures

### *Thread Control Block*

- Executing Context of current thread.
  Normally, a thread only has one specific executing context.
- Thread State.
  - Inactive
  - Running
  - IdleThreadState
- Priority
- Ready Queue pointer

### *Executing Context*

- CPU Regesters

## Functions

### Executing Context

#### Executing Context related function

1. *ReadReg*
2. *CopyReg*
3. *WriteReg*

### thread_ctx Operations

#### thread_ctx related function

1. *SetPriority*
2. *SetTimeoutHandler*
3. *SetThreadState*

#### Ready Queue related function

1. *Enqueue*
2. *Dequeue*
3. *Append*

### Scheduling Mechanisms

#### *SwitchToThread*

Switches the current thread to the speciﬁed one
1. Check the status of the target thread. (Whether the target thread’s SC has budget)
2. Switch current running thread’s executing context
3. *Dequeue* from the ready queue

#### *SwitchToIdleThread*

1. Switch current running thread’s executing context

#### *ChooseThread*

Get the highest priority thread from the ready-queue.
If there exists a thread:
1. Check the status of the target thread.
2. Use *SwitchToThread*
If there does not exist a thread:
1. Use *SwitchToIdleThread*

#### *Schedule*

1. Choose appropriate thread through calling *ChooseThread* (Simple Priority-Based Policy)
2. Switch Scheduling Context (Refill and switch Scheduling Context)
3. Update budgets and set new timer

#### *Suspend*

1. Set thread state
2. *Dequeue* from the ready queue

#### *Restart*

1. Set thread state
2. Check scheduling context 
3. Possible Switch To (If failed then *Enqueue*)

# With Real Time Support

## Overview

1. Support real-time task.

   - For all sporadic tasks (with ddl) and aperiodic tasks (without ddl), they execute through a sporadic server. (Same as seL4-MCS)

   - User-space RT server is responsible to check the scheduability.

     Client passes the scheduling context to the Manager when creating the thread, and the manager will create the corresponding scheduling context and put it into the ready queue.

   - Use sporatic-server algorithm

2. Seperate thread's info into *Executing Context* and *Scheduling Context* 

3. Fixed-priority Preemptive Scheduler

4. Dynamic tick system

   - [ ] It may takes unacceptable time to modify the timer every time.

   

## Data stuctures

### *Thread Control Block*

- Executing Context of current thread.

  Normally, a thread only has one specific executing context.

- Scheduling Context of current thread.

  Can be bound with different scheduling context dynamically. When using IPC to require services from some *Passive Servers*, the thread can unbind its SC and bind the SC to the target server.

- Thread State.

  - Inactive
  - Running
  - Restart
  - IdleThreadState
  
- Priority

- Ready Queue pointer

- Timeout handler

### *Executing Context*

- CPU Regesters
- FPU context

### *Scheduling Context*

- Budget: Worst Case Execution Time
- Period: Control budget replenishing rate
- Consumed: amount 
- Refill: Initially => Budget. Refill the same amount of time after period T every time the thread consume the time.

## Functions

### Executing Context

#### Executing Context related function

1. *ReadReg*
2. *CopyReg*
3. *WriteReg*

### Scheduling Context

#### Scheduling Context related function

1. *UpdateConsumed*: Get consumed time of current SC.
2. *BindTCB*: Bind the SC to a thread_ctx.
3. *UnbindTCB*: Unbind the SC from a thread_ctx.
4. *TransferSC*: Unbind old thread_ctx and bind new thread_ctx

#### Budget Refill related function

1. *RefillSufficient*: Which the budget is sufficient.
2. *RefillReady*: Whether the budget can be consumed.
3. *RefillNew*: Create a new refill.
4. *RefillUpdate*: Update a refill.

#### *UpdateBudget*

1. Charge the consumed budget
2. Update refill budget (Sporadic Server Algorithm)

### thread_ctx Operations

#### thread_ctx related function

1. *SetPriority*
2. *SetTimeoutHandler*
3. *SetThreadState*



#### Ready Queue related function

1. *Enqueue*
2. *Dequeue*
3. *Append*



### Scheduling Mechanisms

#### *SwitchToThread*

Switches the current thread to the speciﬁed one

1. Check the status of the target thread. (Whether the target thread’s SC has budget)
2. Switch current running thread’s executing context
3. *Dequeue* from the ready queue

#### *SwitchToIdleThread*

1. Switch current running thread’s executing context

#### *ChooseThread*

Get the highest priority thread from the ready-queue.

If there exists a thread:

1. Check the status of the target thread.
2. Use *SwitchToThread*

If there does not exist a thread:

1. Use *SwitchToIdleThread*

#### *Schedule*

1. Choose appropriate thread through calling *ChooseThread* (Simple Priority-Based Policy)
2. Switch Scheduling Context (Refill and switch Scheduling Context)
3. Update budgets and set new timer

#### *Suspend*

1. Set thread state
2. *Dequeue* from the ready queue

#### *Restart*

1. Set thread state
2. Check scheduling context 
3. Possible Switch To (If failed then *Enqueue*)
