!------------------------------------------------------------------------------
! 
! !MODULE: comm_mod
!
! !DESCRIPTION:
! Define subroutines that interoperate with MPI C++ routines
!
module comm
use iso_c_binding

interface
  subroutine start_parallel_f(argc,argv) bind(C,name='start_parallel')
    import
    integer    (C_INT)   :: argc
    type       (C_ptr)   :: argv
  end subroutine start_parallel_f

  subroutine end_parallel_f() bind(C,name='end_parallel')
  end subroutine end_parallel_f

   function get_rank_f() result(my_rank) bind(C, name='get_rank_')
     import
     integer    (C_INT)   :: my_rank
   end function get_rank_f
end interface

CONTAINS

! this routine gets argc and argv and puts them in the C-style
subroutine get_command_args(argc,argv)
  integer(C_int), intent(inout) ::argc
  type(C_ptr), allocatable,intent(inout) ::argv(:)
  
  character(len=256,kind=c_char) :: arg
  character(len=1,kind=c_char), pointer :: carg(:)
  integer :: strlen,i,j

   argc = iargc()
   allocate(argv(0:argc+1))

   do i = 0,argc
     call getarg(i,arg)
     strlen = len(trim(arg))
     allocate(carg(0:strlen))
     do j = 0, strlen-1
       carg(j) = arg(j+1:j+1)
     end do
     carg(strlen) = c_null_char
     argv(i) = c_loc(carg(0))
   end do
   argv(argc+1) = C_NULL_ptr
   argc = argc + 1
end subroutine get_command_args

subroutine start_parallel(argc, argv)
  integer    (C_INT), intent(in)   :: argc
  type       (C_ptr), value, intent(in)   :: argv
  call start_parallel_f(argc,argv)
end subroutine start_parallel


subroutine end_parallel()
  call end_parallel_f()
end subroutine end_parallel

        function get_rank() result(my_rank)
           integer    (C_INT)   :: my_rank
           my_rank = get_rank_f()
        end function get_rank


end module comm
