#ifndef __HYPERVISOR_H__
#define __HYPERVISOR_H__

/* Hypervisor base value */
#define HYPERCALL_BASE_VALUE 0x86000000

/* Hypervisor actions (TODO Change name) */
enum hypervisor_actions { HC_IPC = 1, HC_REQUEST_MEM_ACCESS = 2, HC_REVOKE_MEM_ACCESS = 3, HC_GET_CPU_ID = 4 };

/* Hypercall with the specified action and arguments */
int hypercall(enum hypervisor_actions, int arg0, int arg1, int arg2);

#endif