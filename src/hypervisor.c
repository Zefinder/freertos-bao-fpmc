#include <hypervisor.h>

int hypercall(enum hypervisor_actions action, int arg0, int arg1, int arg2) {
    int hypercall_id = HYPERCALL_BASE_VALUE | action;
    int ret;

    __asm__ volatile (
        "hvc #0\n\t"
        : "=r" (ret)
        : "r" (hypercall_id), "r" (arg0), "r" (arg1), "r" (arg2)
    );

    return ret;
}