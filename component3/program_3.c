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

static void dummy() {}
void reboot()
{
    printf("this component is going to be rebooted\n");
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(nInvocationLabels + 1, 0, 0, 0);
    /* seL4_Call(seL4_CapIRQControl, tag); */
    printf("back?????\n");
    while(1){}
}

int main(int argc, char *argv[])
{
    platsupport_serial_setup_bootinfo_failsafe();

    printf("Done, suspend init thread\n");
    reboot();

    seL4_TCB_Suspend(seL4_CapInitThreadTCB);
    return 0;
}
