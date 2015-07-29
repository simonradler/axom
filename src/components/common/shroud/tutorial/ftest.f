!
! Test tutorial wrapper
!

program tester
  use iso_c_binding
  use tutorial_mod
  implicit none

  type(class1) cptr
  real(C_DOUBLE) rv2
  logical rv3
  character(30) rv4, rv4b

  call function1

  cptr = class1_new()
  call cptr%method1

  ! Arguments
  ! Integer and Real
  rv2 = function2(1.5d0, 2)

  ! logical
  rv3 = function3(.false.)

  ! character
  rv4 = function4a("bird", "dog")
  print *, rv4
  call  function4b("bird", "dog", rv4b)
  print *, rv4b



end program tester
