! !module: options_class
!
      module cstring_class
!
! !uses:
      use iso_c_binding

      implicit none

      type, public :: cstring
          type(c_ptr) :: str = c_null_ptr
           contains
             procedure :: print_string
             procedure :: set_string
             procedure :: get_string
      end type cstring
      
      interface
        function alloc_f() result(this)  bind(c,name='_string')
          import
          type (c_ptr) :: this
        end function alloc_f
        
        subroutine print_string_f(this)  bind(c,name='print_string')
          import
          type (c_ptr), value :: this
        end subroutine print_string_f
        
        subroutine set_string_f(this,str)  bind(c,name='set_string')
          import
          type (c_ptr), value :: this
          character(kind=c_char,len=1), target  :: str
        end subroutine set_string_f
        
        subroutine get_string_f(this,str,len)  bind(c,name='get_string')
          import
          type (c_ptr), value :: this
          character(kind=c_char,len=1), target :: str(*)
          integer(c_int), value :: len
        end subroutine get_string_f
        
        subroutine delete_string_f(str)  bind(c,name='delete_string')
          import
          type (c_ptr), value :: str
        end subroutine delete_string_f
      end interface
        
      contains

      type(cstring) function new_cstring()
        type (cstring) :: this
        new_cstring%str = alloc_f()
      end function new_cstring

      subroutine delete_cstring(str)
        type(cstring) :: str
        call delete_string_f(str%str)
      end subroutine delete_cstring
      
      subroutine print_string(this)
        class(cstring) :: this
        call print_string_f(this%str)
      end subroutine print_string
      
      subroutine set_string(this,string)
        class(cstring) :: this
        character(len=*) :: string
        call set_string_f(this%str,trim(string)//c_null_char)
      end subroutine set_string
      
      subroutine get_string(this,string)
        class(cstring) :: this
        character(len=*), intent(inout) :: string

        integer :: l
        l = len(string)
        call get_string_f(this%str,string,l)
      end subroutine get_string
          

    end module cstring_class

!------------------------------------------------------------------------------
! !module: options_class
!
      module option_class
!
! !uses:
      use iso_c_binding

      implicit none

      type, public :: options
          type(c_ptr) :: opt = c_null_ptr
           contains
             procedure :: read_options
             procedure :: add_double
             procedure :: add_float
             procedure :: add_int
             procedure :: add_string
             generic :: add => add_double, add_float, add_int, add_string
             !final :: delete_options
      end type options

      interface new_options
        module procedure alloc
      end interface
        
      interface
        function alloc_f() result(this)  bind(c,name='_options')
          import
          type (c_ptr) :: this
        end function alloc_f
        
        subroutine delete_f(this) bind(c,name='_dealloc')
          import
          type (c_ptr), value :: this
        end subroutine delete_f

        subroutine add_double_f(this, name_, val) bind(c,name='add_double')
          import
          type      (c_ptr),              value,  intent(in) :: this
          character (kind=c_char,len=1),          intent(in) :: name_(*)
          real      (c_double),                   intent(in) :: val
        end subroutine add_double_f
        
        subroutine add_float_f(this, name_, val) bind(c,name='add_float')
          import
          type      (c_ptr),              value,  intent(in) :: this
          character (kind=c_char,len=1),          intent(in) :: name_
          real      (c_float),                    intent(in) :: val
        end subroutine add_float_f
        
        subroutine add_int_f(this, name_, val) bind(c,name='add_int')
          import
          type      (c_ptr),              value,  intent(in) :: this
          character (kind=c_char,len=1),          intent(in) :: name_(*)
          integer   (c_int),                      intent(in) :: val
        end subroutine add_int_f
        
        subroutine add_string_f(this, name_, val) bind(c,name='add_string')
          import
          type      (c_ptr),              value,  intent(in) :: this
          character (kind=c_char),  intent(in) :: name_(*)
          type(c_ptr), value :: val
        end subroutine add_string_f

        subroutine read_options_f(this, argc, argv) bind(c,name='_read_options')
          import
          type      (c_ptr), value,                    intent(in) :: this
          integer   (c_int), value,                    intent(in) :: argc
          type      (c_ptr), value,                    intent(in) :: argv
        end subroutine read_options_f
      
      end interface

!-----------------------------------------------------------------------------
      contains
!-----------------------------------------------------------------------------
      type(options) function alloc()
            alloc%opt =  alloc_f()
      end function alloc
        
      subroutine delete_options(this)
        type(options), intent(in) :: this
        call delete_f(this%opt)
      end subroutine delete_options

      subroutine add_double(this, name_, val)
        class      (options), intent(inout) :: this
        character (len=*),    intent(in) :: name_
        real      (c_double), intent(inout) :: val
        call add_double_f(this%opt,trim(name_)//c_null_char,val)
      end subroutine add_double
    
      subroutine add_float(this, name_, val)
        class      (options), intent(inout) :: this
        character (len=*),    intent(in) :: name_
        real      (c_float),  intent(inout) :: val
        call add_float_f(this%opt,trim(name_)//c_null_char,val)
      end subroutine add_float
    
      subroutine add_int(this, name_, val)
        class      (options), intent(inout) :: this
        character (len=*),    intent(in) :: name_
        integer   (c_int),    intent(inout) :: val
        call add_int_f(this%opt,trim(name_)//c_null_char,val)
      end subroutine add_int
        
      subroutine add_string(this, name_, val)
      use cstring_class
        class    (options),   intent(inout) :: this
        character (len=*),    intent(in) :: name_
        type(cstring), intent(in)  :: val
        call add_string_f(this%opt,trim(name_)//c_null_char,val%str)
      end subroutine add_string

      subroutine read_options(this, argc, argv)
          class     (options),      intent(inout) :: this
          integer   (c_int), value,       intent(in) :: argc
          type      (c_ptr), value,       intent(in) :: argv
          call read_options_f(this%opt,argc,argv)
      end subroutine read_options

      end module option_class
