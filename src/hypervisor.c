#include <hypervisor.h>

uint64_t hypercall(enum hypervisor_actions action, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    uint64_t hypercall_id = HYPERCALL_BASE_VALUE | action;
    uint64_t ret;

    __asm__ volatile(
        "hvc #0\n\t"
        "dsb   SY\n\t"
        "isb\n\t"
        : "=r"(ret)
        : "r"(hypercall_id), "r"(arg0), "r"(arg1), "r"(arg2));

    return ret;
}