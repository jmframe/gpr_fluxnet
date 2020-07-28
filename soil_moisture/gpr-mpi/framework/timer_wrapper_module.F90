!------------------------------------------------------------------------------
! 
! !module: timer_wrapper_module
!
! !description:
! define subroutines that interoperate with mpi c++ routines
!
      module timer_class
      use iso_c_binding

      type, public :: timer
         type(c_ptr) :: tm = c_null_ptr
         contains
             procedure :: timer_start_1
             procedure :: stop
             generic :: start => timer_start_1
             !procedure :: start
             !final :: delete_timer
      end type timer

      interface
        function new_timer_f() result(this) bind(c, name='_timer')
          import
          type(c_ptr) :: this
        end function new_timer_f

        subroutine timer_start_f_1(this, msg) bind(c, name='_timer_start_1')
          import
          type(c_ptr), value  :: this
          character(kind=c_char,len=1), dimension(*), intent(in) :: msg
        end subroutine timer_start_f_1

        !subroutine timer_start_f_2(this, msg, color) bind(c, name='_timer_start_2')
        !  import
        !  type(c_ptr), value :: this
        !  character(kind=c_char,len=1), dimension(*), intent(in) :: msg
        !  character(kind=c_char,len=1), dimension(*), intent(in) :: color
        !end subroutine timer_start_f_2

        subroutine timer_stop_f(this) bind(c,name='_timer_stop')
          import
          type(c_ptr), value :: this
        end subroutine timer_stop_f

        subroutine delete_timer_f(this) bind(c,name='_delete_timer')
          import
          type(c_ptr), value :: this
        end subroutine delete_timer_f
      end interface


   contains
        type(timer) function new_timer()
          new_timer%tm = new_timer_f()
        end function new_timer

        subroutine timer_start_1(this, msg)
          class(timer)  :: this
          character(len=*), intent(in)  :: msg
          call timer_start_f_1(this%tm, trim(msg)//c_null_char)
        end subroutine timer_start_1

!        subroutine timer_start_2(this, msg, color)
!          type(for_timer)  :: this
!          character  (len=*), intent(in)  :: msg
!          character  (len=*), intent(in)  :: color
!          call timer_start_f_2(this%tm, trim(msg)//c_null_char, trim(color)//c_null_char)
!        end subroutine timer_start_2

        subroutine stop(this)
          class(timer)  :: this
          call timer_stop_f(this%tm)
        end subroutine stop

        subroutine delete_timer(this)
          type(timer)  :: this
          call delete_timer_f(this%tm)
        end subroutine delete_timer

      end module timer_class
