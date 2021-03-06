#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/util.h>
#include <sel4utils/util.h>
#include <sel4utils/helpers.h>
#include <sel4platsupport/platsupport.h>
#include <simple-default/simple-default.h>
#include <sel4utils/sel4_zf_logif.h>

extern seL4_CPtr free_slot;
extern seL4_CPtr untyped_cap_start;

void create_child(seL4_CPtr root_cnode, seL4_CPtr root_vspace, seL4_CPtr root_tcb);
void sayhello();

char* buffer[4096];

seL4_BootInfo *bi;

int main(int argc, char *argv[])
{
    platsupport_serial_setup_bootinfo_failsafe();

    create_child(seL4_CapInitThreadCNode, seL4_CapInitThreadVSpace, seL4_CapInitThreadTCB);
    return 0;
}

void create_child(seL4_CPtr root_cnode, seL4_CPtr root_vspace, seL4_CPtr root_tcb)
{
    seL4_CPtr new_tcb = free_slot;
    seL4_CPtr new_cnode = free_slot + 1;

    /* seL4_DebugNameThread(new_tcb, "Hello"); */

    // printf("tcb %lu cnode %lu\n", new_tcb, new_cnode);
    // create the cnode
    int error = seL4_Untyped_Retype(untyped_cap_start, seL4_CapTableObject, 10, root_cnode, 0, 0, new_cnode, 1);
    ZF_LOGF_IF(error, "Failed to create child cnode");

    error = seL4_Untyped_Retype(untyped_cap_start, seL4_TCBObject, seL4_TCBBits, root_cnode, 0, 0, new_tcb, 1);
    ZF_LOGF_IF(error, "Failed to create tcb");

    error = seL4_TCB_Configure(new_tcb, 0, new_cnode, 0, root_vspace, 0, 0, 0);
    ZF_LOGF_IF(error, "Failed to configure tcb");

    error = seL4_TCB_SetPriority(new_tcb, root_tcb, 253);
    ZF_LOGF_IF(error, "Failed to set priority");

    seL4_UserContext regs = {0};
    error = seL4_TCB_ReadRegisters(new_tcb, 0, 0, sizeof(regs) / sizeof(seL4_Word), &regs);
    ZF_LOGF_IF(error, "Failed to read registers");
    sel4utils_arch_init_local_context((void*)sayhello, 0, 0, 0, buffer, &regs);

    error = seL4_TCB_WriteRegisters(new_tcb, 0, 0, sizeof(regs) / sizeof(seL4_Word), &regs);
    ZF_LOGF_IF(error, "Failed to write registers");

    error = seL4_TCB_Resume(new_tcb);
    ZF_LOGF_IF(error, "Failed to resume tcb");

    printf("Done, suspend init thread\n");
    seL4_TCB_Suspend(seL4_CapInitThreadTCB);
}

void sayhello()
{
    printf("This is the sayhello program from another capdl loader and then i will say: \n");
    printf("hello\n");
    printf("After me saying hello, the timer client from another capdl loader should resume after 3 second sleep\n");
    while (1) {}
}
