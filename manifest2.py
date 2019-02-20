import pickle
from capdl.Spec import Spec
from capdl.Object import CNode, Endpoint, Frame, TCB, PML4, Untyped
from capdl.Cap import Cap
from capdl.Allocator import ObjectAllocator, CSpaceAllocator, AddressSpaceAllocator


CNODE_SIZE=20
# NOTE: this guard_size must be 64 - cnode_size
GUARD_SIZE=64-CNODE_SIZE

cnode_program_1 = CNode("cnode_program_1_component", CNODE_SIZE)
ep = Endpoint("endpoint")

ipc_program_1_obj = Frame("ipc_program_1_obj", 4096)
vspace_program_1 = PML4("vspace_program_1")

tcb_program_1 = TCB("tcb_program_1_component",ipc_buffer_vaddr= 0x0,ip= 0x0,sp= 0x0,elf= "program_1_component",prio= 254,max_prio= 254,affinity= 0,init= [])

cap_tcb = Cap(tcb_program_1)
cap_cnode = Cap(cnode_program_1, guard_size=GUARD_SIZE)

cnode_program_1["0x1"] = Cap(tcb_program_1)
cnode_program_1["0x2"] = Cap(cnode_program_1, guard_size=GUARD_SIZE)
cnode_program_1["0x3"] = Cap(vspace_program_1)

# create a list of untyped memory which will passed into children components
untyped_list = []
for i in range(0, 20):
    temp = Untyped('untyped_program_1_{}'.format(i), size_bits=24)
    untyped_list.append(temp)
    cnode_program_1["0x{:X}".format(0x13 + i)] = Cap(temp, read=True, write=True, grant=True)

tcb_program_1['cspace'] = Cap(cnode_program_1, guard_size=GUARD_SIZE)
tcb_program_1['vspace'] = Cap(vspace_program_1)
tcb_program_1['ipc_buffer_slot'] = Cap(ipc_program_1_obj, read=True, write=True)

stack_0_program_1_obj = Frame("stack_0_program_1_obj", 4096)
stack_1_program_1_obj = Frame("stack_1_program_1_obj", 4096)
stack_2_program_1_obj = Frame("stack_2_program_1_obj", 4096)
stack_3_program_1_obj = Frame("stack_3_program_1_obj", 4096)
stack_4_program_1_obj = Frame("stack_4_program_1_obj", 4096)
stack_5_program_1_obj = Frame("stack_5_program_1_obj", 4096)
stack_6_program_1_obj = Frame("stack_6_program_1_obj", 4096)
stack_7_program_1_obj = Frame("stack_7_program_1_obj", 4096)
stack_8_program_1_obj = Frame("stack_8_program_1_obj", 4096)
stack_9_program_1_obj = Frame("stack_9_program_1_obj", 4096)

obj = set([
cnode_program_1,
ep,
ipc_program_1_obj,
stack_0_program_1_obj,
stack_1_program_1_obj,
stack_2_program_1_obj,
stack_3_program_1_obj,
stack_4_program_1_obj,
stack_5_program_1_obj,
stack_6_program_1_obj,
stack_7_program_1_obj,
stack_8_program_1_obj,
stack_9_program_1_obj,
vspace_program_1,
tcb_program_1,
])
obj.update(untyped_list)

spec = Spec('x86_64')
spec.objs = obj
objects = ObjectAllocator()
objects.spec.arch  = 'x86_64'
objects.counter = len(obj)
objects.merge(spec)

program_1_alloc = CSpaceAllocator(cnode_program_1)
program_1_alloc.slot = 8

cspaces = {'program_1_component':program_1_alloc}

program_1_addr_alloc = AddressSpaceAllocator('addr_allocator_program_1', vspace_program_1)
program_1_addr_alloc._symbols = {}

addr_spaces = {
    'program_1_component': program_1_addr_alloc,
               }

cap_symbols = {
    'program_1_component':
	    [('tcb', 1),
	     ('cnode', 2),
         ('untyped_cap_start', 0x13),
         ('free_slot', 0x13 + 20),
        ],
               }

region_symbols = {}

elfs =  {
    'program_1_component': {'passive': False, 'filename': 'program_1.c'},
         }

print(pickle.dumps((objects, cspaces, addr_spaces, cap_symbols, region_symbols, elfs)))

