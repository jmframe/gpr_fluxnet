!------------------------------------------------------------------------------
! 
! !MODULE: gpr_wrapper_module
!
! !DESCRIPTION:
! Define subroutines/functions that interoperate with C (C++).
!
      module gpr_class
!
! !USES:
      use iso_c_binding

      implicit none

       type, public :: gaussian_process_regression
          type(C_ptr) :: gpr = C_NULL_ptr
           CONTAINS
             procedure :: maximize_marginal_probability
             procedure :: write_gpr_state
             procedure :: set_signal_to_noise
             procedure :: set_signal_to_kernel
             procedure :: set_kernel_nugget
             generic, public :: make_prediction => make_prediction_2, make_prediction_3
             procedure, private :: make_prediction_2, make_prediction_3
             !final :: delete_gaussian_process_regression
      end type gaussian_process_regression

      INTERFACE
        function new_gaussian_process_regression_f_1(dimx, dim_train, &
                         kernel_type, training_input_file, &
                         training_targets_file,preprocess_method) &
                         result(this) bind(C, name='_gaussian_process_regression_1')
          import
          type     (C_ptr)  :: this
          integer  (C_INT), value,                    intent(in) :: dimx
          integer  (C_INT), value,                    intent(in) :: dim_train
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: kernel_type
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: training_input_file
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: training_targets_file
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: preprocess_method
        end function new_gaussian_process_regression_f_1

        function new_gaussian_process_regression_f_2(dimx, dim_train, &
                         kernel_type, training_input, &
                        training_targets, preprocess_method) &
                        result(this) bind(C, name='_gaussian_process_regression_2')
          import
          type     (C_ptr)                                       :: this
          integer  (C_INT), value,                    intent(in) :: dimx
          integer  (C_INT), value,                    intent(in) :: dim_train
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: kernel_type
          real     (kind=C_DOUBLE),     dimension(*), intent(in) :: training_input
          real     (kind=C_DOUBLE),     dimension(*), intent(in) :: training_targets
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: preprocess_method
        end function new_gaussian_process_regression_f_2

        function new_gaussian_process_regression_f_3(gpr_state_file) result(this) &
                           bind(C, name='_gaussian_process_regression_3')
          import
          type     (C_ptr)                                       :: this
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: gpr_state_file
        end function new_gaussian_process_regression_f_3

        function maximize_marginal_probability_f(this, stepsize,maxiter, &
                          tol, tol_gradient) result(res) &
                          bind(C,name='_maximize_marginal_probability')
          import
          type     (C_ptr),    value             :: this
          real     (C_DOUBLE), value, intent(in) :: stepsize
          integer  (C_INT),    value, intent(in) :: maxiter
          real     (C_DOUBLE), value, intent(in) :: tol
          real     (C_DOUBLE), value, intent(in) :: tol_gradient
          real     (kind=8)                      :: res
        end function maximize_marginal_probability_f

        subroutine make_prediction_f_2(this, input_filename, num_inputs, &
                          target_filename,  target_covariance_filename) &
                          bind(C,name='_make_prediction_2')
          import
          type     (C_ptr),                    value             :: this
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: input_filename
          integer  (C_INT),                    value, intent(in) :: num_inputs
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: target_filename
          character(kind=C_CHAR,len=1), dimension(*), intent(in) :: target_covariance_filename
        end subroutine make_prediction_f_2

        subroutine make_prediction_f_3(this, pred_inputs, num_inputs, &
                          prediction_targets) &
                          bind(C,name='_make_prediction_3')
          import
          type     (C_ptr),     value             :: this
          real     (C_DOUBLE),  dimension(*), intent(in) :: pred_inputs
          integer  (C_INT),     value, intent(in) :: num_inputs
          real     (C_DOUBLE),  dimension(*), intent(out) :: prediction_targets
        end subroutine make_prediction_f_3
  
        !--------------------
        ! GPR write state
        !--------------------
        subroutine write_gpr_state_f(this, fileName) &
                              bind(C,name='_write_gpr_state')
          import
          type       (C_ptr),    value     :: this
          character(kind=C_CHAR,len=1), dimension(*) :: fileName
        end subroutine write_gpr_state_f

        !-------------------
        ! Set GPR Parameters
        !-------------------
        subroutine set_signal_to_noise_f(this, signal_to_noise) &
                                      bind(C,name='_set_signal_to_noise')
            import
            type       (C_ptr),  value              :: this
            real     (C_DOUBLE), value , intent(in) :: signal_to_noise
        end subroutine set_signal_to_noise_f

        subroutine set_signal_to_kernel_f(this, signal_to_kernel) &
                                     bind(C,name='_set_signal_to_kernel')
            import
            type       (C_ptr),  value              :: this
            real     (C_DOUBLE), value , intent(in) :: signal_to_kernel
        end subroutine set_signal_to_kernel_f

        subroutine set_kernel_nugget_f(this, kernel_nugget) &
                                     bind(C,name='_set_kernel_nugget')
            import
            type       (C_ptr),  value              :: this
            real     (C_DOUBLE), value , intent(in) :: kernel_nugget
        end subroutine set_kernel_nugget_f

        !-------------------
        ! Delete GPR 
        !-------------------
        subroutine delete_f(this) bind(C,name='_delete')
          import
          type       (C_ptr),    value     :: this
        end subroutine delete_f

        subroutine print_f(this) bind(C,name='_print')
          import
          type       (C_ptr),    value     :: this
        end subroutine print_f

       END INTERFACE

       INTERFACE new_gaussian_process_regression
          MODULE PROCEDURE new_gaussian_process_regression_1
          MODULE PROCEDURE new_gaussian_process_regression_2
          MODULE PROCEDURE new_gaussian_process_regression_3
       END INTERFACE 


!-----------------------------------------------------------------------------
CONTAINS
!------------------------------------------------------------------------------

      !-------------------------------------------------
      ! Fortran wrapper routines to interface C wrappers
      !-------------------------------------------------

        !--------------------
        ! ---> GPR Allocation
        !--------------------
        TYPE(gaussian_process_regression) function new_gaussian_process_regression_1(dimx, dim_train, kernel_type, &
            training_input_file, training_targets_file, preprocess_method)
            integer    (C_INT), intent(in)  :: dimx
            integer    (C_INT), intent(in)  :: dim_train
            character  (len=*), intent(in)  :: kernel_type
            character  (len=*), intent(in)  :: training_input_file
            character  (len=*), intent(in)  :: training_targets_file
            character  (len=*), intent(in)  :: preprocess_method
            new_gaussian_process_regression_1%gpr = &
                        new_gaussian_process_regression_f_1(dimx, dim_train, &
                              TRIM(kernel_type)//C_NULL_CHAR, &
                              TRIM(training_input_file)//C_NULL_CHAR, &
                              TRIM(training_targets_file)//C_NULL_CHAR, &
                              TRIM(preprocess_method)//C_NULL_CHAR)
        end function new_gaussian_process_regression_1

        TYPE(gaussian_process_regression) function new_gaussian_process_regression_2(dimx, dim_train, kernel_type, &
            training_input, training_targets, preprocess_method)
            integer     (C_INT), intent(in)  :: dimx
            integer     (C_INT), intent(in)  :: dim_train
            character   (len=*), intent(in)  :: kernel_type
            real     (C_DOUBLE), intent(in)  :: training_input(*)
            real     (C_DOUBLE), intent(in)  :: training_targets(*)
            character   (len=*), intent(in)  :: preprocess_method
            new_gaussian_process_regression_2%gpr = &
                        new_gaussian_process_regression_f_2(dimx, dim_train, &
                              TRIM(kernel_type)//C_NULL_CHAR, &
                              training_input, training_targets, &
                              TRIM(preprocess_method)//C_NULL_CHAR)
        end function new_gaussian_process_regression_2

        TYPE(gaussian_process_regression) function new_gaussian_process_regression_3(gpr_state_file)
              character  (len=*), intent(in)  :: gpr_state_file
              new_gaussian_process_regression_3%gpr = &
                          new_gaussian_process_regression_f_3( TRIM(gpr_state_file)//C_NULL_CHAR )
        end function new_gaussian_process_regression_3

        function maximize_marginal_probability(this, stepsize, maxiter, tol, tol_gradient) result(res)
            class(gaussian_process_regression), intent(inout) :: this
            real       (C_DOUBLE), intent(in)     :: stepsize
            integer    (C_INT),    intent(in)     :: maxiter
            real       (C_DOUBLE), intent(in)     :: tol
            real       (C_DOUBLE), intent(in)     :: tol_gradient
            real       (kind=8)                   :: res
            res  =  maximize_marginal_probability_f(this%gpr,stepsize,maxiter,tol,tol_gradient)
        end function maximize_marginal_probability
  
        subroutine make_prediction_2(this, input_filename, num_inputs, &
                                    target_filename, target_covariance_filename)
            class(gaussian_process_regression), intent(inout) :: this
            character   (len=*), intent(in)  :: input_filename
            integer     (C_INT), intent(in)  :: num_inputs
            character   (len=*), intent(in)  :: target_filename
            character   (len=*), intent(in)  :: target_covariance_filename
            call  make_prediction_f_2(this%gpr, input_filename, num_inputs, &
                                      target_filename, target_covariance_filename)
        end subroutine make_prediction_2

        subroutine make_prediction_3(this, pred_inputs, &
                                    num_inputs, prediction_targets)
            class(gaussian_process_regression), intent(inout) :: this
            real     (C_DOUBLE), intent(in) :: pred_inputs(*)
            integer  (C_INT),    intent(in) :: num_inputs
            real     (C_DOUBLE), intent(out) :: prediction_targets(*)
            call  make_prediction_f_3(this%gpr, pred_inputs, num_inputs, &
                                      prediction_targets)
        end subroutine make_prediction_3

        subroutine write_gpr_state(this, fileName)
            class(gaussian_process_regression), intent(inout) :: this
            character   (len=*)  :: fileName
            call write_gpr_state_f(this%gpr, TRIM(fileName)//C_NULL_CHAR)
        end subroutine write_gpr_state

        subroutine set_signal_to_noise(this, signal_to_noise)
            class(gaussian_process_regression), intent(inout) :: this
            real     (C_DOUBLE), intent(in) :: signal_to_noise
            call set_signal_to_noise_f(this%gpr, signal_to_noise)
        end subroutine set_signal_to_noise
  
        subroutine set_signal_to_kernel(this, signal_to_kernel)
            class(gaussian_process_regression), intent(inout) :: this
            real     (C_DOUBLE), intent(in) :: signal_to_kernel
            call set_signal_to_kernel_f(this%gpr, signal_to_kernel)
        end subroutine set_signal_to_kernel

        subroutine set_kernel_nugget(this, kernel_nugget)
            class(gaussian_process_regression), intent(inout) :: this
            real     (C_DOUBLE), intent(in) :: kernel_nugget
            call set_kernel_nugget_f(this%gpr, kernel_nugget)
        end subroutine set_kernel_nugget

        subroutine delete_gaussian_process_regression(this)
            type(gaussian_process_regression), intent(inout) :: this
            call delete_f(this%gpr)
        end subroutine delete_gaussian_process_regression

      end module gpr_class
!-------------------------------------------------------------------------------
