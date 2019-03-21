import pickle
from capdl.Spec import Spec
from capdl.Object import CNode, Endpoint, Frame, TCB, PML4, Untyped, IRQControl, PageDirectory
from capdl.Cap import Cap
from capdl.Allocator import ObjectAllocator, CSpaceAllocator, AddressSpaceAllocator


CNODE_SIZE=8
# NOTE: this guard_size must be 64 - cnode_size
GUARD_SIZE=32-CNODE_SIZE

cnode_program_3 = CNode("cnode_program_3", CNODE_SIZE)
cnode_program_2 = CNode("cnode_program_2", CNODE_SIZE)
ep = Endpoint("endpoint")

cap_irq_control = Cap(IRQControl('irq_control'))

cnode_program_3["0x1"] = Cap(ep, read=True, write=True, grant=True)
cnode_program_3["0x2"] = Cap(cnode_program_3, guard_size=GUARD_SIZE)
cnode_program_3["0x3"] = cap_irq_control

cnode_program_2["0x1"] = Cap(ep, read=True, write=True, grant=True)
cnode_program_2["0x2"] = Cap(cnode_program_2, guard_size=GUARD_SIZE)
#  cnode_program_2["0x3"] = cap_irq_control

ipc_program_3_obj = Frame ("ipc_program_3_obj", 4096)
ipc_program_2_obj = Frame ("ipc_program_2_obj", 4096)
vspace_program_3 = PageDirectory("vspace_program_3")
vspace_program_2 = PageDirectory("vspace_program_2")

tcb_program_3 = TCB ("tcb_program_3",ipc_buffer_vaddr= 0x0,ip= 0x0,sp= 0x0,elf= "program_3",prio= 252,max_prio= 252,affinity= 0,init= [])
tcb_program_2 = TCB ("tcb_program_2",ipc_buffer_vaddr= 0x0,ip= 0x0,sp= 0x0,elf= "program_2",prio= 252,max_prio= 252,affinity= 0,init= [])

tcb_program_3['cspace'] = Cap(cnode_program_3, guard_size=GUARD_SIZE)
tcb_program_3['vspace'] = Cap(vspace_program_3)
tcb_program_3['ipc_buffer_slot'] = Cap(ipc_program_3_obj, read=True, write=True)

tcb_program_2['cspace'] = Cap(cnode_program_2, guard_size=GUARD_SIZE)
tcb_program_2['vspace'] = Cap(vspace_program_2)
tcb_program_2['ipc_buffer_slot'] = Cap(ipc_program_2_obj, read=True, write=True)

stack_0_program_3_obj = Frame("stack_0_program_3_obj", 4096)
stack_1_program_3_obj = Frame("stack_1_program_3_obj", 4096)
stack_2_program_3_obj = Frame("stack_2_program_3_obj", 4096)
stack_3_program_3_obj = Frame("stack_3_program_3_obj", 4096)
stack_4_program_3_obj = Frame("stack_4_program_3_obj", 4096)
stack_5_program_3_obj = Frame("stack_5_program_3_obj", 4096)
stack_6_program_3_obj = Frame("stack_6_program_3_obj", 4096)
stack_7_program_3_obj = Frame("stack_7_program_3_obj", 4096)
stack_8_program_3_obj = Frame("stack_8_program_3_obj", 4096)
stack_9_program_3_obj = Frame("stack_9_program_3_obj", 4096)

stack_0_program_2_obj = Frame("stack_0_program_2_obj", 4096)
stack_1_program_2_obj = Frame("stack_1_program_2_obj", 4096)
stack_2_program_2_obj = Frame("stack_2_program_2_obj", 4096)
stack_3_program_2_obj = Frame("stack_3_program_2_obj", 4096)
stack_4_program_2_obj = Frame("stack_4_program_2_obj", 4096)
stack_5_program_2_obj = Frame("stack_5_program_2_obj", 4096)
stack_6_program_2_obj = Frame("stack_6_program_2_obj", 4096)
stack_7_program_2_obj = Frame("stack_7_program_2_obj", 4096)
stack_8_program_2_obj = Frame("stack_8_program_2_obj", 4096)
stack_9_program_2_obj = Frame("stack_9_program_2_obj", 4096)

obj = set([
    cnode_program_3,
    cnode_program_2,
    ep,
    ipc_program_3_obj,
    ipc_program_2_obj,
    stack_0_program_3_obj,
    stack_0_program_2_obj,
    stack_1_program_3_obj,
    stack_1_program_2_obj,
    stack_2_program_3_obj,
    stack_2_program_2_obj,
    stack_3_program_3_obj,
    stack_3_program_2_obj,
    stack_4_program_3_obj,
    stack_4_program_2_obj,
    stack_5_program_3_obj,
    stack_5_program_2_obj,
    stack_6_program_3_obj,
    stack_6_program_2_obj,
    stack_7_program_3_obj,
    stack_7_program_2_obj,
    stack_8_program_3_obj,
    stack_8_program_2_obj,
    stack_9_program_3_obj,
    stack_9_program_2_obj,
    vspace_program_3,
    vspace_program_2,
    tcb_program_3,
    tcb_program_2,
])
spec = Spec('aarch32')
spec.objs = obj

objects = ObjectAllocator()
objects.counter = len(obj)
objects.spec.arch  = 'aarch32'
objects.merge(spec)

program_3_alloc = CSpaceAllocator(cnode_program_3)
program_3_alloc.slot = 4
program_2_alloc = CSpaceAllocator(cnode_program_2)
program_2_alloc.slot = 4
cspaces = {'program_3':program_3_alloc, 'program_2': program_2_alloc}


program_3_addr_alloc = AddressSpaceAllocator(None, vspace_program_3)
program_3_addr_alloc._symbols = {
    'mainIpcBuffer': ([4096], [Cap(ipc_program_3_obj, read=True, write=True)]),
    'stack': ([4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096,],
              [Cap(stack_0_program_3_obj, read=True, write=True),
               Cap(stack_1_program_3_obj, read=True, write=True),
               Cap(stack_2_program_3_obj, read=True, write=True),
               Cap(stack_3_program_3_obj, read=True, write=True),
               Cap(stack_4_program_3_obj, read=True, write=True),
               Cap(stack_5_program_3_obj, read=True, write=True),
               Cap(stack_6_program_3_obj, read=True, write=True),
               Cap(stack_7_program_3_obj, read=True, write=True),
               Cap(stack_8_program_3_obj, read=True, write=True),
               Cap(stack_9_program_3_obj, read=True, write=True),
               ])}

program_2_addr_alloc = AddressSpaceAllocator(None, vspace_program_2)
program_2_addr_alloc._symbols = {
    'mainIpcBuffer': ([4096], [Cap(ipc_program_2_obj, read=True, write=True)]),
    'stack': ([4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096,],
              [Cap(stack_0_program_2_obj, read=True, write=True),
               Cap(stack_1_program_2_obj, read=True, write=True),
               Cap(stack_2_program_2_obj, read=True, write=True),
               Cap(stack_3_program_2_obj, read=True, write=True),
               Cap(stack_4_program_2_obj, read=True, write=True),
               Cap(stack_5_program_2_obj, read=True, write=True),
               Cap(stack_6_program_2_obj, read=True, write=True),
               Cap(stack_7_program_2_obj, read=True, write=True),
               Cap(stack_8_program_2_obj, read=True, write=True),
               Cap(stack_9_program_2_obj, read=True, write=True),
               ])}

addr_spaces = {
    'program_3': program_3_addr_alloc,
    'program_2': program_2_addr_alloc,
}

cap_symbols = {
    'program_3':
    [('endpoint', 1),
     ('cnode', 2),
     ('badged_endpoint', 3)],
    'program_2':
    [('endpoint', 1),
     ('cnode', 2),
     ('badged_endpoint', 3)],
}

region_symbols = {
    'program_3': [('stack', 65536, 'size_12bit'), ('mainIpcBuffer', 4096, 'size_12bit')],
    'program_2': [('stack', 65536, 'size_12bit'), ('mainIpcBuffer', 4096, 'size_12bit')]
}

elfs =  {
    'program_3': {'passive': False, 'filename': 'program_3.c'},
    'program_2': {'passive': False, 'filename': 'program_2.c'},
}

print(pickle.dumps((objects, cspaces, addr_spaces, cap_symbols, region_symbols, elfs)))
