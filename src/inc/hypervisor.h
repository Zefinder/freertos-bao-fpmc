#ifndef __HYPERVISOR_H__
#define __HYPERVISOR_H__

/* Hypervisor base value (aarch64) */
#define HYPERCALL_BASE_VALUE 0x86000000 | 0x40000000

/* Hypervisor actions (TODO Change name) */
enum hypervisor_actions { HC_IPC = 1, HC_REQUEST_MEM_ACCESS = 2, HC_REVOKE_MEM_ACCESS = 3, HC_GET_CPU_ID = 4, HC_NOTIFY_CPU = 5 };

/* Hypercall with the specified action and arguments */
int hypercall(enum hypervisor_actions, int arg0, int arg1, int arg2);

/* Macros for memory request and revoke */
#define request_memory_access(prio) hypercall(HC_REQUEST_MEM_ACCESS, prio, 0, 0)
#define revoke_memory_access()      hypercall(HC_REVOKE_MEM_ACCESS, 0, 0, 0)

#endif