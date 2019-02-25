#include <autoconf.h>

#include <assert.h>
#include <inttypes.h>
#include <limits.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <elf/elf.h>
#include <sel4platsupport/platsupport.h>
#include <cpio/cpio.h>
#include <simple-default/simple-default.h>

#include <vka/kobject_t.h>
#include <utils/util.h>
#include <sel4/sel4.h>
#include <sel4utils/sel4_zf_logif.h>
#include <sel4utils/util.h>
#include <sel4utils/helpers.h>

extern char _component_cpio[];
static seL4_CPtr free_slot_start, free_slot_end;
static seL4_BootInfo *bootinfo;
#define copy_addr (ROUND_UP(((uintptr_t)_end) + (PAGE_SIZE_4K * 3), 0x1000000))
static char copy_addr_with_pt[PAGE_SIZE_4K] __attribute__((aligned(PAGE_SIZE_4K)));

static void init_system(void);
/* static void elf_load_frames(const char *elf_name, CDL_ObjID pd, CDL_Model *spec, seL4_BootInfo *bootinfo); */
static void create_child(seL4_CPtr root_cnode, seL4_CPtr root_vspace, seL4_CPtr root_tcb);
void sayhello();

#define PML4_SLOT(vaddr) ((vaddr >> (seL4_PDPTIndexBits + seL4_PageDirIndexBits + seL4_PageTableIndexBits + seL4_PageBits)) & MASK(seL4_PML4IndexBits))
#define PDPT_SLOT(vaddr) ((vaddr >> (seL4_PageDirIndexBits + seL4_PageTableIndexBits + seL4_PageBits)) & MASK(seL4_PDPTIndexBits))
#define PD_SLOT(vaddr)   ((vaddr >> (seL4_PageTableIndexBits + seL4_PageBits)) & MASK(seL4_PageDirIndexBits))
#define PT_SLOT(vaddr)   ((vaddr >> seL4_PageBits) & MASK(seL4_PageTableIndexBits))
#define PGD_SLOT(vaddr) ((vaddr >> (seL4_PUDIndexBits + seL4_PageDirIndexBits + seL4_PageTableIndexBits + seL4_PageBits)) & MASK(seL4_PGDIndexBits))
#define PUD_SLOT(vaddr) ((vaddr >> (seL4_PageDirIndexBits + seL4_PageTableIndexBits + seL4_PageBits)) & MASK(seL4_PUDIndexBits))

char* buffer[4096];

static seL4_CPtr untyped_cptrs[CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS];

extern char __executable_start[];
extern char _end[];

static seL4_Word curr_untyped = 0;

static void
sort_untypeds(seL4_BootInfo *bootinfo)
{
    seL4_CPtr untyped_start = bootinfo->untyped.start;
    seL4_CPtr untyped_end = bootinfo->untyped.end;

    ZF_LOGD("Sorting untypeds...\n");

    seL4_Word count[CONFIG_WORD_SIZE] = {0};

    // Count how many untypeds there are of each size.
    for (seL4_Word untyped_index = 0; untyped_index != untyped_end - untyped_start; untyped_index++) {
        if (!bootinfo->untypedList[untyped_index].isDevice) {
            count[bootinfo->untypedList[untyped_index].sizeBits] += 1;
        }
    }

    // Calculate the starting index for each untyped.
    seL4_Word total = 0;
    for (seL4_Word size = CONFIG_WORD_SIZE - 1; size != 0; size--) {
        seL4_Word oldCount = count[size];
        count[size] = total;
        total += oldCount;
    }

    // Store untypeds in untyped_cptrs array.
    for (seL4_Word untyped_index = 0; untyped_index != untyped_end - untyped_start; untyped_index++) {
        if (bootinfo->untypedList[untyped_index].isDevice) {
            ZF_LOGD("Untyped %3d (cptr=%p) (addr=%p) is of size %2d. Skipping as it is device\n",
                    untyped_index, (void*)(untyped_start + untyped_index),
                    (void*)bootinfo->untypedList[untyped_index].paddr,
                    bootinfo->untypedList[untyped_index].sizeBits);
        } else {
            ZF_LOGD("Untyped %3d (cptr=%p) (addr=%p) is of size %2d. Placing in slot %d...\n",
                    untyped_index, (void*)(untyped_start + untyped_index),
                    (void*)bootinfo->untypedList[untyped_index].paddr,
                    bootinfo->untypedList[untyped_index].sizeBits,
                    count[bootinfo->untypedList[untyped_index].sizeBits]);

            untyped_cptrs[count[bootinfo->untypedList[untyped_index].sizeBits]] = untyped_start +  untyped_index;
            count[bootinfo->untypedList[untyped_index].sizeBits] += 1;
        }
    }
}

static void init_elf(const char* elf_name)
{
    seL4_Word elf_size;
    void *elf_file = cpio_get_file(_component_cpio, elf_name, &elf_size);
    if (!elf_file) {
        ZF_LOGF("Didn't find the elf %s", elf_name);
    }

    ZF_LOGD("   ELF loading %s (from %p)... \n", elf_name, elf_file);
    for (int i = 0; i < elf_getNumProgramHeaders(elf_file); i++) {
        ZF_LOGD("    to %p... ", (void*)(uintptr_t)elf_getProgramHeaderVaddr(elf_file, i));

        size_t f_len = elf_getProgramHeaderFileSize(elf_file, i);
        uintptr_t dest = elf_getProgramHeaderVaddr(elf_file, i);
        uintptr_t src = (uintptr_t) elf_file + elf_getProgramHeaderOffset(elf_file, i);

        if (elf_getProgramHeaderType(elf_file, i) != PT_LOAD) {
            ZF_LOGD("Skipping non loadable header");
            continue;
        }

        // here we should create frames for loadable sections

        ZF_LOGD("f_len is 0x%x", f_len);
        ZF_LOGD("dest is %p", dest);
        ZF_LOGD("src is %p", src);

        seL4_Word num_pages = ROUND_UP(f_len, PAGE_SIZE_4K) / PAGE_SIZE_4K;
        ZF_LOGD("# pages needed is %u", ROUND_UP(f_len, PAGE_SIZE_4K) / PAGE_SIZE_4K);

        int start_page_slot = free_slot_start;
        int error;
        for (int i = 0; i < num_pages; ++i) {
            do {
                error = seL4_Untyped_Retype(untyped_cptrs[curr_untyped], seL4_X86_4K, 0, seL4_CapInitThreadCNode, 0, 0, free_slot_start++, 1);

                if (error) {
                    curr_untyped++;
                }
            } while (error);
        }

        int end_page_slot = free_slot_start;

        ZF_LOGD("For this section the first page is in slot %u the last page is in slot %u", start_page_slot, end_page_slot);


        uintptr_t vaddr = dest;

    }

    /* seL4_Word num_pages = (size) / 0x1000 + 1; */
    /* ZF_LOGD("# of pages the elf needs is %u", num_pages); */

    /* int start_page_slot = free_slot_start; */

    /* int error; */
    /* for (int i = 0; i < num_pages; ++i) { */
    /* do { */
    /* error = seL4_Untyped_Retype(untyped_cptrs[curr_untyped], seL4_X86_4K, 0, seL4_CapInitThreadCNode, 0, 0, free_slot_start++, 1); */

    /* if (error) { */
    /* curr_untyped++; */
    /* } else { */
    /* ZF_LOGD("created page out of untyped %u in slot %u", curr_untyped, free_slot_start - 1); */
    /* } */
    /* } while (error); */
    /* } */

    /* int end_page_slot = free_slot_start; */

    /* ZF_LOGD("For this elf the first page is in slot %u the last page is in slot %u", start_page_slot, end_page_slot); */
}

static void
init_elfs()
{
    ZF_LOGD("Initialising ELFs...\n");
    ZF_LOGD(" Available ELFs:\n");
    for (int j = 0; ; j++) {
        const char *name = NULL;
        unsigned long size;
        void *ptr = cpio_get_entry(_component_cpio, j, &name, &size);
        if (ptr == NULL) {
            break;
        }
        ZF_LOGD("  %d: %s, offset: %p, size: %lu\n", j, name,
                (void*)((uintptr_t)ptr - (uintptr_t)_component_cpio), size);

        init_elf(name);
    }
}

int main(int argc, char *argv[])
{
    platsupport_serial_setup_bootinfo_failsafe();

    printf("archive addr is %p\n", &_component_cpio);

    seL4_Word addr = 0x400000ul;
    ZF_LOGD("the pml4_slot is %u", PML4_SLOT(addr));
    ZF_LOGD("the pdpt_slot is %u", PDPT_SLOT(addr));
    ZF_LOGD("the pd_slot is %u", PD_SLOT(addr));
    ZF_LOGD("the pt_slot is %u", PT_SLOT(addr));

    init_system();
    sort_untypeds(bootinfo);
    init_elfs();

    seL4_DebugDumpScheduler();

    seL4_CPtr new_pd = free_slot_start++;
    seL4_CPtr new_pt = free_slot_start++;

    int error = seL4_Untyped_Retype(untyped_cptrs[curr_untyped], seL4_X64_PML4Object, 0, seL4_CapInitThreadCNode, 0, 0, new_pd, 1);
    ZF_LOGF_IF(error, "Failed to create new vspace root");
    error = seL4_Untyped_Retype(untyped_cptrs[curr_untyped], seL4_X86_PDPTObject, 0, seL4_CapInitThreadCNode, 0, 0, new_pt, 1);
    ZF_LOGF_IF(error, "Failed to create vspace page table");
    error = seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool, new_pd);
    ZF_LOGF_IF(error, "Failed to assign page to asid_pool");



    /* assert(!"stop"); */

    create_child(seL4_CapInitThreadCNode, new_pd, seL4_CapInitThreadTCB);

    seL4_DebugDumpScheduler();

    printf("Done, suspend init thread\n");
    seL4_TCB_Suspend(seL4_CapInitThreadTCB);

    return 0;
}

void init_copy_frame()
{
    /* An original frame will be mapped, backing copy_addr_with_pt. For
     * correctness we should unmap this before mapping into this
     * address. We locate the frame cap by looking in boot info
     * and knowing that the userImageFrames are ordered by virtual
     * address in our address space. The flush is probably not
     * required, but doesn't hurt to be cautious.
     */

    /* Find the number of frames in the user image according to
     * bootinfo, and compare that to the number of frames backing
     * the image computed by comparing start and end symbols. If
     * these numbers are different, assume the image was padded
     * to the left. */

    ZF_LOGD("in bootinfo start = %u, end = %u", bootinfo->userImageFrames.start, bootinfo->userImageFrames.end);
    ZF_LOGD("global var start = %p, end = %p", &__executable_start, &_end);
    unsigned int num_user_image_frames_reported =
        bootinfo->userImageFrames.end - bootinfo->userImageFrames.start;
    unsigned int num_user_image_frames_measured =
        (ROUND_UP((uintptr_t)&_end, PAGE_SIZE_4K) -
         (uintptr_t)&__executable_start) / PAGE_SIZE_4K;

    ZF_LOGD("reported %u frames, measured %u frames", num_user_image_frames_reported, num_user_image_frames_measured);

    if (num_user_image_frames_reported < num_user_image_frames_measured) {
        ZF_LOGD("Too few frames caps in bootinfo to back user image");
        return;
    }

    /* Here we tried to put the extra bytes before
     * the __executable_start symbol */
    size_t additional_user_image_bytes =
        (num_user_image_frames_reported - num_user_image_frames_measured) * PAGE_SIZE_4K;

    if (additional_user_image_bytes > (uintptr_t)&__executable_start) {
        ZF_LOGD("User image padding too high to fit before start symbol");
        return;
    }

    uintptr_t lowest_mapped_vaddr =
        (uintptr_t)&__executable_start - additional_user_image_bytes;
    ZF_LOGD("lowest mapped vaddr is %p", lowest_mapped_vaddr);
    ZF_LOGD("lowest mapped # frame is %u", lowest_mapped_vaddr / PAGE_SIZE_4K);

    ZF_LOGD("copy_addr_with_pt addr is %p", (uintptr_t)copy_addr_with_pt);
    ZF_LOGD("copy_addr_with_pt # frame is %u", (uintptr_t)copy_addr_with_pt / PAGE_SIZE_4K);

    seL4_CPtr copy_addr_frame = bootinfo->userImageFrames.start +
                                ((uintptr_t)copy_addr_with_pt) / PAGE_SIZE_4K -
                                lowest_mapped_vaddr / PAGE_SIZE_4K;
    /* We currently will assume that we are on a 32-bit platform
     * that has a single PD, followed by all the PTs. So to find
     * our PT in the paging objects list we just need to add 1
     * to skip the PD */

    /* bootinfo->userImagePaging.start is the cap to the PD
     * so bootinfo->userImagePaging.start + 1 is the start of PT
     * */
    seL4_CPtr copy_addr_pt = bootinfo->userImagePaging.start + 1 +
                             PD_SLOT(((uintptr_t)copy_addr)) - PD_SLOT(((uintptr_t)&__executable_start));
#if defined(CONFIG_ARCH_X86_64) || defined(CONFIG_ARCH_AARCH64)
    /* guess that there is one PDPT and PML4 on x86_64 or one PGD and PUD on aarch64 */
    copy_addr_pt += 2;
#endif

    int error;

    ZF_LOGD("size of copy_addr_with_pt is %u", sizeof(copy_addr_with_pt));
    // for each page of copy_addr_with_pt
    for (int i = 0; i < sizeof(copy_addr_with_pt) / PAGE_SIZE_4K; i++) {
#ifdef CONFIG_ARCH_ARM
        error = seL4_ARM_Page_Unify_Instruction(copy_addr_frame + i, 0, PAGE_SIZE_4K);
        ZF_LOGF_IFERR(error, "");
#endif
        error = seL4_ARCH_Page_Unmap(copy_addr_frame + i);
        ZF_LOGF_IFERR(error, "");

        if ((i + 1) % BIT(seL4_PageTableIndexBits) == 0) {
            error = seL4_ARCH_PageTable_Unmap(copy_addr_pt + i / BIT(seL4_PageTableIndexBits));
            ZF_LOGF_IFERR(error, "");
        }
    }
}


static void
parse_bootinfo(seL4_BootInfo *bootinfo)
{
    ZF_LOGD("Parsing bootinfo...\n");

    free_slot_start = bootinfo->empty.start;
    free_slot_end = bootinfo->empty.end;

    /* When using libsel4platsupport for printing support, we end up using some
     * of our free slots during serial port initialisation. Skip over these to
     * avoid failing our own allocations. Note, this value is just hardcoded
     * for the amount of slots this initialisation currently uses up.
     * JIRA: CAMKES-204.
     */
    free_slot_start += 16;

    /* We need to be able to actual store caps to the maximum number of objects
     * we may be dealing with.
     * This check can still pass and initialisation fail as we need extra slots for duplicates
     * for CNodes and TCBs.
     */
    ZF_LOGD("free_slot_end = %u, free_slot_start = %u", free_slot_end, free_slot_start);
    ZF_LOGD("# extra caps = %u", CONFIG_CAPDL_LOADER_MAX_OBJECTS);
    assert(free_slot_end - free_slot_start >= CONFIG_CAPDL_LOADER_MAX_OBJECTS);

    ZF_LOGD("  %ld free cap slots, from %ld to %ld\n", (long)(free_slot_end - free_slot_start), (long)free_slot_start, (long)free_slot_end);

    int num_untyped = bootinfo->untyped.end - bootinfo->untyped.start;
    ZF_LOGD("  Untyped memory (%d)\n", num_untyped);
    for (int i = 0; i < num_untyped; i++) {
        uintptr_t ut_paddr = bootinfo->untypedList[i].paddr;
        uintptr_t ut_size = bootinfo->untypedList[i].sizeBits;
        bool ut_isDevice = bootinfo->untypedList[i].isDevice;
        ZF_LOGD("    0x%016" PRIxPTR " - 0x%016" PRIxPTR " (%s)\n", ut_paddr,
                ut_paddr + BIT(ut_size), ut_isDevice ? "device" : "memory");
    }
    ZF_LOGD("Loader is running in domain %d\n", bootinfo->initThreadDomain);
}

static void init_system(void)
{
    bootinfo = platsupport_get_bootinfo();
    simple_t simple;
    simple_default_init_bootinfo(&simple, bootinfo);
    parse_bootinfo(bootinfo);
    init_copy_frame();
}


static void
create_child(seL4_CPtr root_cnode, seL4_CPtr new_vspace, seL4_CPtr root_tcb)
{
    seL4_CPtr new_tcb = free_slot_start++;
    seL4_CPtr new_cnode = free_slot_start++;

    // create the cnode
    int error = seL4_Untyped_Retype(untyped_cptrs[curr_untyped], seL4_CapTableObject, 10, root_cnode, 0, 0, new_cnode, 1);
    ZF_LOGF_IF(error, "Failed to create child cnode");

    error = seL4_Untyped_Retype(untyped_cptrs[curr_untyped], seL4_TCBObject, seL4_TCBBits, root_cnode, 0, 0, new_tcb, 1);
    ZF_LOGF_IF(error, "Failed to create tcb");

    seL4_DebugNameThread(new_tcb, "capdl-app");

    seL4_DebugDumpScheduler();

    error = seL4_TCB_Configure(new_tcb, 0, new_cnode, 0, new_vspace, 0, 0, 0);
    ZF_LOGF_IF(error, "Failed to configure tcb");

    error = seL4_TCB_SetPriority(new_tcb, root_tcb, 254);
    ZF_LOGF_IF(error, "Failed to set priority");

    seL4_UserContext regs = {0};
    error = seL4_TCB_ReadRegisters(new_tcb, 0, 0, sizeof(regs) / sizeof(seL4_Word), &regs);
    ZF_LOGF_IF(error, "Failed to read registers");
    sel4utils_arch_init_local_context((void*)sayhello, 0, 0, 0, buffer, &regs);

    error = seL4_TCB_WriteRegisters(new_tcb, 0, 0, sizeof(regs) / sizeof(seL4_Word), &regs);
    ZF_LOGF_IF(error, "Failed to write registers");

    error = seL4_TCB_Resume(new_tcb);
    ZF_LOGF_IF(error, "Failed to resume tcb");
}

void sayhello()
{
    printf("hello\n");
    while (1);
}


/* static void */
/* elf_load_frames(const char *elf_name, CDL_ObjID pd, CDL_Model *spec, seL4_BootInfo *bootinfo) */
/* { */
/* // bootinfo is not used in this function actually */
/* unsigned long elf_size; */
/* void *elf_file = cpio_get_file(_capdl_archive, elf_name, &elf_size); */

/* if (elf_file == NULL) { */
/* ZF_LOGF("ELF file %s not found", elf_name); */
/* } */

/* if (elf_checkFile(elf_file) != 0) { */
/* ZF_LOGF("Unable to read elf file %s at %p", elf_name, elf_file); */
/* } */

/* ZF_LOGD("   ELF loading %s (from %p)... \n", elf_name, elf_file); */

/* for (int i = 0; i < elf_getNumProgramHeaders(elf_file); i++) { */
/* ZF_LOGD("    to %p... ", (void*)(uintptr_t)elf_getProgramHeaderVaddr(elf_file, i)); */

/* size_t f_len = elf_getProgramHeaderFileSize(elf_file, i); */
/* uintptr_t dest = elf_getProgramHeaderVaddr(elf_file, i); */
/* uintptr_t src = (uintptr_t) elf_file + elf_getProgramHeaderOffset(elf_file, i); */

/* //Skip non loadable headers */
/* if (elf_getProgramHeaderType(elf_file, i) != PT_LOAD) { */
/* ZF_LOGD("Skipping non loadable header\n"); */
/* continue; */
/* } */
/* uintptr_t vaddr = dest; */

/* while (vaddr < dest + f_len) { */
/* ZF_LOGD("."); */

/* [> map frame into the loader's address space so we can write to it <] */
/* seL4_CPtr sel4_page = get_frame_cap(pd, vaddr, spec); */
/* seL4_CPtr sel4_page_pt = get_frame_pt(pd, vaddr, spec); */
/* size_t sel4_page_size = get_frame_size(pd, vaddr, spec); */

/* seL4_ARCH_VMAttributes attribs = seL4_ARCH_Default_VMAttributes; */
/* #ifdef CONFIG_ARCH_ARM */
/* attribs |= seL4_ARM_ExecuteNever; */
/* #endif */

/* int error = seL4_ARCH_Page_Map(sel4_page, seL4_CapInitThreadPD, (seL4_Word)copy_addr, */
/* seL4_ReadWrite, attribs); */
/* if (error == seL4_FailedLookup) { */
/* error = seL4_ARCH_PageTable_Map(sel4_page_pt, seL4_CapInitThreadPD, (seL4_Word)copy_addr, */
/* seL4_ARCH_Default_VMAttributes); */
/* ZF_LOGF_IFERR(error, ""); */
/* error = seL4_ARCH_Page_Map(sel4_page, seL4_CapInitThreadPD, (seL4_Word)copy_addr, */
/* seL4_ReadWrite, attribs); */
/* } */
/* if (error) { */
/* Try and retrieve some useful information to help the user
 * diagnose the error.
 */
/* ZF_LOGD("Failed to map frame "); */
/* seL4_ARCH_Page_GetAddress_t addr UNUSED = seL4_ARCH_Page_GetAddress(sel4_page); */
/* if (addr.error) { */
/* ZF_LOGD("<unknown physical address (error = %d)>", addr.error); */
/* } else { */
/* ZF_LOGD("%p", (void*)addr.paddr); */
/* } */
/* ZF_LOGD(" -> %p (error = %d)\n", (void*)copy_addr, error); */
/* ZF_LOGF_IFERR(error, ""); */
/* } */

/* [> copy until end of section or end of page <] */
/* size_t len = dest + f_len - vaddr; */
/* if (len > sel4_page_size - (vaddr % sel4_page_size)) { */
/* len = sel4_page_size - (vaddr % sel4_page_size); */
/* } */
/* memcpy((void *) (copy_addr + vaddr % sel4_page_size), (void *) (src + vaddr - dest), len); */

/* #ifdef CONFIG_ARCH_ARM */
/* error = seL4_ARM_Page_Unify_Instruction(sel4_page, 0, sel4_page_size); */
/* ZF_LOGF_IFERR(error, ""); */
/* #endif */
/* error = seL4_ARCH_Page_Unmap(sel4_page); */
/* ZF_LOGF_IFERR(error, ""); */

/* if (sel4_page_pt != 0) { */
/* error = seL4_ARCH_PageTable_Unmap(sel4_page_pt); */
/* ZF_LOGF_IFERR(error, ""); */
/* } */

/* vaddr += len; */
/* } */

/* Overwrite the section type so that next time this section is
 * encountered it will be skipped as it is not considered loadable. A
 * bit of a hack, but fine for now.
 */
/* ZF_LOGD(" Marking header as loaded\n"); */
/* if (((struct Elf32_Header*)elf_file)->e_ident[EI_CLASS] == ELFCLASS32) { */
/* elf32_getProgramHeaderTable(elf_file)[i].p_type = PT_NULL; */
/* } else if (((struct Elf64_Header*)elf_file)->e_ident[EI_CLASS] == ELFCLASS64) { */
/* elf64_getProgramHeaderTable(elf_file)[i].p_type = PT_NULL; */
/* } */
/* } */
/* } */
