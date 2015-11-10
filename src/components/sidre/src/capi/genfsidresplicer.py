#
# Routines to generate splicers for wrappers.
# Used to generate several variations of a routine for Fortran.
# Similar to templates in C++.
#
from __future__ import print_function
import sys

types = (
    ( 'int',    'integer(C_INT)',  'ATK_C_INT_T',    'CONDUIT_NATIVE_INT_DATATYPE_ID'),
    ( 'long',   'integer(C_LONG)', 'ATK_C_LONG_T',   'CONDUIT_NATIVE_LONG_DATATYPE_ID'),
    ( 'float',  'real(C_FLOAT)',   'ATK_C_FLOAT_T',  'CONDUIT_NATIVE_FLOAT_DATATYPE_ID'),
    ( 'double', 'real(C_DOUBLE)',  'ATK_C_DOUBLE_T', 'CONDUIT_NATIVE_DOUBLE_DATATYPE_ID'),
)

# XXX - only doing 0-d and 1-d for now
maxdims = 1

def XXnum_metabuffers():
    return len(types) * (maxdims + 1) # include scalars
######################################################################

def create_array_view(d):
    # typename - part of function name
    # nd       - number of dimensions
    # f_type   - fortran type
    # shape     - :,:, to match nd
    if d['rank'] == 0:
        size = '1_C_LONG'
    else:
        size = 'size(value, kind=1_C_LONG)'

    return """
! Generated by genfsidresplicer.py
function datagroup_create_array_view_{typename}_{nd}(group, name, value) result(rv)
    use iso_c_binding
    implicit none

    interface
       function ATK_create_array_view(group, name, lname, addr, type, nitems) result(rv) bind(C,name="ATK_create_array_view")
       use iso_c_binding
       type(C_PTR), value, intent(IN)     :: group
       character(kind=C_CHAR), intent(IN) :: name(*)
       integer(C_INT), value, intent(IN)  :: lname
       type(C_PTR), value,     intent(IN) :: addr
       integer(C_INT), value, intent(IN)  :: type
       integer(C_LONG), value, intent(IN) :: nitems
       type(C_PTR) rv
       end function ATK_create_array_view
    end interface
    external :: ATK_C_LOC

    class(datagroup), intent(IN) :: group
    character(*), intent(IN) :: name
    {f_type}, target, intent(IN) :: value{shape}
    integer(C_INT) :: lname
    type(dataview) :: rv
    integer(C_LONG) :: nitems
    integer(C_INT), parameter :: type = {atk_type}
    type(C_PTR) addr

    lname = len_trim(name)
    nitems = {size}
    call ATK_C_LOC(value, addr)
    rv%voidptr = ATK_create_array_view(group%voidptr, name, lname, addr, type, nitems)
end function datagroup_create_array_view_{typename}_{nd}""".format(
        size=size, **d)


def create_allocatable_view(d):
    # typename - part of function name
    # nd       - number of dimensions
    # f_type   - fortran type
    # shape     - :,:, to match nd
    callname = 'ATK_create_allocatable_view_{typename}_{nd}'.format(**d)
    return """
! Generated by genfsidresplicer.py
function datagroup_create_allocatable_view_{typename}_{nd}(group, name, value) result(rv)
    use iso_c_binding
    implicit none
    class(datagroup), intent(IN) :: group
    character(*), intent(IN) :: name
    {f_type}, allocatable, intent(IN) :: value{shape}
    integer(C_INT) :: lname
    type(dataview) :: rv
    type(C_PTR) :: addr
    integer(C_INT), parameter :: rank = {rank}
    integer(C_INT), parameter :: itype = {atk_type}

    lname = len_trim(name)
    call c_loc_allocatable(value, addr)
    rv%voidptr = ATK_create_fortran_allocatable_view(group%voidptr, name, lname, addr, itype, rank)
end function datagroup_create_allocatable_view_{typename}_{nd}""".format(callname=callname, **d)

def print_get_value(d):
    # typename - part of function name
    # nd       - number of dimensions
    # f_type   - fortran type
    # shape     - :,:, to match nd
    if d['rank'] == 0:
        return """
! Generated by genfsidresplicer.py
subroutine dataview_get_value_{typename}_{nd}{suffix}(view, value)
    use iso_c_binding
    implicit none
    class(dataview), intent(IN) :: view
    {f_type}, pointer, intent(OUT) :: value{shape}
    type(C_PTR) cptr

    cptr = view%get_data_pointer()
    call c_f_pointer(cptr, value)
end subroutine dataview_get_value_{typename}_{nd}{suffix}""".format(**d)

    elif d['rank'] == 1:
        return """
! Generated by genfsidresplicer.py
subroutine dataview_get_value_{typename}_{nd}{suffix}(view, value)
    use iso_c_binding
    implicit none
    class(dataview), intent(IN) :: view
    {f_type}, pointer, intent(OUT) :: value{shape}
    type(C_PTR) cptr
    integer(C_SIZE_T) nelems

    cptr = view%get_data_pointer()
    nelems = view%get_number_of_elements()
    call c_f_pointer(cptr, value, [ nelems ])
end subroutine dataview_get_value_{typename}_{nd}{suffix}""".format(**d)
    else:
        raise RuntimeError("rank too large in print_get_value")


def type_bound_procedure_part(d):
    return 'procedure :: {stem}_{typename}_{nd}{suffix} => {wrap_class}_{stem}_{typename}_{nd}{suffix}'.format(**d)

def type_bound_procedure_generic(d):
    return '{stem}_{typename}_{nd}{suffix}'.format(**d)

def type_bound_procedure_generic_post(lines, generics, stem):
    lines.append('generic :: %s => &' % stem)
    for gen in generics[:-1]:
        lines.append('    ' + gen + ',  &')
    lines.append('    ' + generics[-1])


def foreach_value(lines, fcn, **kwargs):
    """ Call fcn once for each type, appending to lines.
    kwargs - additional values for format dictionary.
    """
    shape = []
    lbound = []
    for nd in range(maxdims + 1):
        shape.append(':')
        lbound.append('lbound(value,%d)' % (nd+1))
    d = dict(
        suffix=''     # suffix of function name
    )
    d.update(kwargs)
    indx = 0
    for typetuple in types:
        d['typename'], d['f_type'], d['atk_type'], d['cpp_type'] = typetuple

        # scalar values
        # XXX - generic does not distinguish between pointer and non-pointer
#        d['rank'] = -1
#        d['nd'] = 'scalar'
#        d['shape'] = ''
#        lines.append(fcn(d))

        # scalar pointers
        d['index'] = indx
        indx += 1
        d['rank'] = 0
        d['nd'] = 'scalar'
        d['shape'] = ''
        d['lower_bound'] = ''
        lines.append(fcn(d))

        for nd in range(1,maxdims+1):
            d['index'] = indx
            indx += 1
            d['rank'] = nd
            d['nd'] = '%dd' % nd
            d['shape'] = '(' + ','.join(shape[:nd]) + ')'
            d['lower_bound'] = '(' + ','.join(lbound[:nd]) + ')'
            lines.append(fcn(d))

def print_lines(printer, fcn, **kwargs):
    """Print output using printer function.
    [Used with cog]
    """
    lines = []
    foreach_value(lines, fcn, **kwargs)
    for line in lines:
        printer(line)

#----------------------------------------------------------------------

def print_switch(printer, calls):
    """Print a switch statement on type and rank.
    Caller must set fileds in d:
      prefix = call or assignment
                  'call foo'
                  'nitems = foo'
      args   = arguments to function, must include parens.
                  '(args)'
                  ''           -- subroutine with no arguments
    """
    d = {}
    printer('  switch(type)')
    printer('  {')
    for typetuple in types:
        d['typename'], f_type, atk_type, cpp_type = typetuple
        printer('  case %s:' % cpp_type)
        printer('    switch(rank)')
        printer('    {')
        for nd in range(0,maxdims+1):
            if nd == 0:
                d['nd'] = 'scalar'
            else:
                d['nd'] = '%dd' % nd
            printer('    case %d:' % nd)
            for ca in calls:
                d['prefix'] = ca[0]
                d['macro'] = '{prefix}_{typename}_{nd}'.format(**d).upper()
                printer('      ' + ca[1].format(**d) + ';')
            printer('      break;')
        printer('    default:')
        printer('      break;')
        printer('    }')
        printer('    break;')
    printer('  default:')
    printer('    break;')
    printer('  }')

#----------------------------------------------------------------------

def gen_fortran():
    """Generate splicers used by Shroud.
    """
    print('! Generated by genfsidresplicer.py')


    print('! splicer begin class.DataStore.module_top')
    print('interface c_loc_allocatable')
    print_lines(print, iface_atk_c_loc_allocatable)
    print('end interface c_loc_allocatable')
    print('! splicer end class.DataStore.module_top')
    print('')



    # DataGroup
    lines = []

    generics = []
    extra = dict(
        wrap_class='datagroup',
        stem='create_allocatable_view',
        )
    foreach_value(lines, type_bound_procedure_part, **extra)
    foreach_value(generics, type_bound_procedure_generic, **extra)
    type_bound_procedure_generic_post(lines, generics, 'create_allocatable_view')

    generics = []
    extra = dict(
        wrap_class='datagroup',
        stem='create_array_view',
        )
    foreach_value(lines, type_bound_procedure_part, **extra)
    foreach_value(generics, type_bound_procedure_generic, **extra)
    type_bound_procedure_generic_post(lines, generics, 'create_array_view')

    print('! splicer begin class.DataGroup.type_bound_procedure_part')
    for line in lines:
        print(line)
    print('! splicer end class.DataGroup.type_bound_procedure_part')

    print()
    print('------------------------------------------------------------')
    print()

    print('! splicer begin class.DataGroup.additional_functions')
    lines = []
    foreach_value(lines, create_allocatable_view)
    foreach_value(lines, create_array_view)
    for line in lines:
        print(line)
    print('! splicer end class.DataGroup.additional_functions')


    # DataView
    extra = dict(
        wrap_class='dataview',
        stem='get_value',
        suffix='_ptr',
        )
    lines = []
    foreach_value(lines, type_bound_procedure_part, **extra)
    generics = []
    foreach_value(generics, type_bound_procedure_generic, **extra)
    type_bound_procedure_generic_post(lines, generics, 'get_value')

    print('! splicer begin class.DataView.type_bound_procedure_part')
    for line in lines:
        print(line)
    print('! splicer end class.DataView.type_bound_procedure_part')

    print()
    print('------------------------------------------------------------')
    print()

    print('! splicer begin class.DataView.additional_functions')
    lines = []
    foreach_value(lines, print_get_value, suffix='_ptr')
    for line in lines:
        print(line)
    print('! splicer end class.DataView.additional_functions')

######################################################################
#
# Create functions to return the address of an allocatable array as a type(C_PTR)
#

def iface_atk_c_loc_allocatable(d):
    return """
   subroutine atk_c_loc_allocatable_{typename}_{nd}(variable, addr)
     use iso_c_binding
     {f_type}, allocatable, intent(IN) :: variable{shape}
     type(C_PTR), intent(OUT) :: addr
   end subroutine atk_c_loc_allocatable_{typename}_{nd}""".format(**d)

def print_atk_c_loc_allocatable(d):
    """Write C++ routine to accept Fortran allocatable.
    """
    name = 'atk_c_loc_allocatable_{typename}_{nd}'.format(**d)
    return """
void FC_GLOBAL({lower},{upper})
  (void * allocatable, void * * addr)
{{
  *addr = allocatable;
}}""".format(lower=name.lower(), upper=name.upper())
# XXX remove cast after native types are resolved

######################################################################

def print_atk_size_allocatable(d):
    """Write Fortran routine to return size of allocatable.
    """
    if d['rank'] == 0:
        return """
function atk_size_allocatable_{typename}_{nd}(array) result(rv)
    use iso_c_binding
    implicit none
    {f_type}, allocatable, intent(IN) :: array{shape}
    integer(C_SIZE_T) :: rv
    rv = 1
end function atk_size_allocatable_{typename}_{nd}""".format(**d)
    else:
        return """
function atk_size_allocatable_{typename}_{nd}(array) result(rv)
    use iso_c_binding
    implicit none
    {f_type}, allocatable, intent(IN) :: array{shape}
    integer(C_SIZE_T) :: rv
    if (allocated(array)) then
        rv = size(array)
    else
        rv = 0
    endif
end function atk_size_allocatable_{typename}_{nd}""".format(**d)

def print_atk_size_allocatable_header(d):
    """Write C++ declarations for Fortran routines.
    """
    name = 'size_allocatable_{typename}_{nd}'.format(**d)
    return """#define {upper} FC_GLOBAL(atk_{lower},ATK_{upper})
size_t {upper}(void * array);
""".format(lower=name.lower(), upper=name.upper())

def SizeAllocatable(printer):
    calls = [ ('size_allocatable', 'nitems = {macro}(array)') ]
    printer('{')
    printer('  size_t nitems = 0;')
    print_switch(printer, calls)
    printer('  return nitems;')
    printer('}')

######################################################################

def print_atk_address_allocatable(d):
    """Write Fortran routine to return address of allocatable.
    Use C_LOC and add TARGET attribute
    """
    return """
subroutine atk_address_allocatable_{typename}_{nd}(array, addr)
    use iso_c_binding
    implicit none
    {f_type}, allocatable, intent(IN), target :: array{shape}
    type(C_PTR), intent(OUT) :: addr
    addr = c_loc(array)
end subroutine atk_address_allocatable_{typename}_{nd}""".format(**d)

def print_atk_address_allocatable_header(d):
    name = 'address_allocatable_{typename}_{nd}'.format(**d)
    return """#define {upper} FC_GLOBAL(atk_{lower},ATK_{upper})
void {upper}(void * array, void * * addr);
""".format(lower=name.lower(), upper=name.upper())

def AddressAllocatable(printer):
    calls = [ ('address_allocatable', '{macro}(array, &addr)') ]
    printer('{')
    printer('  void * addr = ATK_NULLPTR;')
    print_switch(printer, calls)
    printer('  return addr;')
    printer('}')

######################################################################

def print_atk_allocate_allocatable(d):
    """Write Fortran routine to allocate an allocatable.
    """
    if d['rank'] == 0:
        size = '';
    elif d['rank'] == 1:
        size = '(nitems)'
    else:
        raise NotImplementedError
    

    return """
subroutine atk_allocate_allocatable_{typename}_{nd}(array, nitems)
    use iso_c_binding
    implicit none
    {f_type}, allocatable, intent(OUT), target :: array{shape}
    integer(C_INT), intent(IN) :: nitems
    allocate(array{size})
end subroutine atk_allocate_allocatable_{typename}_{nd}""".format(size=size, **d)

def print_atk_allocate_allocatable_header(d):
    name = 'allocate_allocatable_{typename}_{nd}'.format(**d)
    return """#define {upper} FC_GLOBAL(atk_{lower},ATK_{upper})
void {upper}(void * array, long * nitems);
""".format(lower=name.lower(), upper=name.upper())

def AllocateAllocatable(printer):
    calls = [
        ('allocate_allocatable', '{macro}(array, &nitems)'),
        ('address_allocatable', '{macro}(array, &addr)'),
        ]
    printer('{')
    printer('  void * addr = ATK_NULLPTR;')
    print_switch(printer, calls)
    printer('  return addr;')
    printer('}')

######################################################################

def print_atk_deallocate_allocatable(d):
    """Write Fortran routine to deallocate an allocatable.
    """
    return """
subroutine atk_deallocate_allocatable_{typename}_{nd}(array)
    use iso_c_binding
    implicit none
    {f_type}, allocatable, intent(INOUT), target :: array{shape}
    deallocate(array)
end subroutine atk_deallocate_allocatable_{typename}_{nd}""".format(**d)

def print_atk_deallocate_allocatable_header(d):
    name = 'deallocate_allocatable_{typename}_{nd}'.format(**d)
    return """#define {upper} FC_GLOBAL(atk_{lower},ATK_{upper})
void {upper}(void * array);
""".format(lower=name.lower(), upper=name.upper())

def DeallocateAllocatable(printer):
    calls = [
        ('deallocate_allocatable', '{macro}(array)'),
        ]
    printer('{')
    print_switch(printer, calls)
    printer('  return;')
    printer('}')

######################################################################

def print_atk_reallocate_allocatable(d):
    """Write Fortran routine to allocate an allocatable.
    """
    if d['rank'] == 0:
        size = '';
    elif d['rank'] == 1:
        size = '(nitems)'
    else:
        raise NotImplementedError
    

    return """
subroutine atk_reallocate_allocatable_{typename}_{nd}(array, nitems)
    use iso_c_binding
    implicit none
    {f_type}, allocatable, intent(INOUT), target :: array{shape}
    integer(C_INT), value, intent(IN) :: nitems
    allocate(array{size})
! XXX move_alloc ...
end subroutine atk_reallocate_allocatable_{typename}_{nd}""".format(size=size, **d)

def print_atk_reallocate_allocatable_header(d):
    name = 'reallocate_allocatable_{typename}_{nd}'.format(**d)
    return """#define {upper} FC_GLOBAL(atk_{lower},ATK_{upper})
void {upper}(void * array, long nitems);
""".format(lower=name.lower(), upper=name.upper())

######################################################################

if __name__ == '__main__':
    try:
        cmd = sys.argv[1]
    except IndexError:
        raise RuntimeError("Missing command line argument")

    if cmd == 'fortran':
        # fortran splicers
        gen_fortran()
    elif cmd == 'test':
        AllocateAllocatable(print)
    else:
        raise RuntimeError("Unknown command")
