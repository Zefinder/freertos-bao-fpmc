#ifndef __HYPERVISOR_H__
#define __HYPERVISOR_H__

#include <FreeRTOS.h>

/* Hypervisor base value (aarch64) */
#define HYPERCALL_BASE_VALUE 0x86000000 | 0x40000000

/* Hypervisor actions (TODO Change name) */
enum hypervisor_actions
{
    HC_IPC = 1,
    HC_REQUEST_MEM_ACCESS = 2,
    HC_REVOKE_MEM_ACCESS = 3,
    HC_GET_CPU_ID = 4,
    HC_NOTIFY_CPU = 5,
    HC_EMPTY_CALL = 6,
    HC_REQUEST_MEM_ACCESS_TIMER = 7,
    HC_DISPLAY_RESULTS = 8,
    HC_MEASURE_IPI = 9,
    HC_REVOKE_MEM_ACCESS_TIMER = 10,
    HC_UPDATE_MEM_ACCESS = 11
};

/* Hypercall with the specified action and arguments */
uint64_t hypercall(enum hypervisor_actions action, uint64_t arg0, uint64_t arg1, uint64_t arg2);

/* Macros for memory request and revoke */
#define request_memory_access(prio, wcet) hypercall(HC_REQUEST_MEM_ACCESS, prio, wcet, 0)
#define revoke_memory_access()            hypercall(HC_REVOKE_MEM_ACCESS, 0, 0, 0)
#define update_memory_access(prio)        hypercall(HC_UPDATE_MEM_ACCESS, prio, 0, 0)

#endif