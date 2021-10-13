# CHCORE Performance Reference

## Syscall

### x86

In x86 (test with KVM and timer disabled)

|   OS   | Chcore w/o KPTI | Linux with KPTI | Chcore with KPTI/PCID |
| :----: | :-------------: | :-------------: | :-------------------: |
| Cycles |       112       |       531       |          445          |

### Hikey970

|   OS   | Chcore w/o KPTI |
| :----: | :-------------: |
| Cycles |       145       |



## Context Switch

### x86 w/o KPTI/PCID

|           Operation           | Cycles | Delta |
| :---------------------------: | :----: | :---: |
| Get cpu id and update budget  |  118   |   6   |
|        Eret_to_thread         |  136   |  18   |
| `sched_enqueue` and `sched()` |  211   |  75   |
|        switch vmspace         |  575   |  364  |

### hikey970

|           Operation           | Cycles | Delta |
| :---------------------------: | :----: | :---: |
| Get cpu id and update budget  |  175   |  30   |
|        Eret_to_thread         |  180   |   5   |
| `sched_enqueue` and `sched()` |  432   |  252  |
|        switch vmspace         |  1004  |  572  |

## MZY trampoline IPC

### x86 (KVM)

|   OS   | CHCore w/o KPTI |
| :----: | :-------------: |
| Cycles |      1639       |


Breakdown

|           Operation           | Cycles | Delta |
| :---------------------------: | :----: | :---: |
|       Empty syscall           |  112   |  112  |
|       obj_get & put           |  237   |  125  |
|       ipc_send_cap            |  388   |  151  |
|       copy_to_user            |  532   |  144  |
|   migrate_to_server(no cs)    |  540   |   8   |
|   syscall + ipc_ret (2 cs)    |  1639  | 1099  |

2 * vmspace switch dominate the performance.
