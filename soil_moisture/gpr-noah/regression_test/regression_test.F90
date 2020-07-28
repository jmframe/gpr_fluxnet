!------------------------------------------------------------------------------
!BOP
!
! !DESCRIPTION:
! Program for performing regression tests.
! It mimics a similar C++ program (reference).

      program for_regressionTest

      use iso_c_binding
      use gpr_class
      use timer_class
      use option_class
      use comm

      implicit none

      real(kind=8), parameter :: pi = 3.141592653589793d0
      integer,      parameter :: num = 20
      integer,      parameter :: num_samples = 20
      real(kind=8)       :: stepsize      
      real(kind=8)       :: tol_line      
      real(kind=8)       :: tol_gradient  
      integer            :: maxiter       

      character(len=256) :: fn_training_inputs_1D  = './regression_test/training_inputs_1D.bin'
      character(len=256) :: fn_training_targets_1D = './regression_test/training_targets_1D.bin'
      character(len=256) :: fn_training_inputs_2D  = './regression_test/training_inputs_2D.bin'
      character(len=256) :: fn_training_targets_2D = './regression_test/training_targets_2D.bin'

      integer      :: i, j, n, nump,  u, my_rank
      real(kind=8) :: res, x, msqe, y, h

      type(gaussian_process_regression) :: gpr, gprp, gprn
      type(gaussian_process_regression) :: gpr2D, gpr2Dn

      real(kind=8), allocatable ::    predInputs1D(:),    predInputs2D(:)
      real(kind=8), allocatable ::   predTargets1D(:),   predTargets2D(:)
      real(kind=8), allocatable :: predTrueValue1D(:), predTrueValue2D(:)

      type(for_timer) :: tm

      ! command line containters
      type(c_ptr), allocatable, target :: argv(:)
      integer :: argc

      
!EOP
!------------------------------------------------------------------------------
!BOC
      ! start mpi
      call start_parallel(argc,argv)

      tm = new_timer()
      my_rank = get_rank()

      h = (num_samples*2.0*PI/20.0 ) / num

      PRINT*, '--------------------------------------------'
      PRINT*, '---> Performing 1D Regression Test Case <---'
      PRINT*, '--------------------------------------------'

      ALLOCATE(predInputs1D(num))
      ALLOCATE(predTargets1D(num))
      ALLOCATE(predTrueValue1D(num))

      DO i = 1, num
         x                = h*(i-1)
         predInputs1D(i)    = x
         predTrueValue1D(i) = x*sin(x)
      ENDDO

      ! Initialize the GPR
      stepsize     = 1.0d-4
      maxiter      = 1000
      tol_line     = 0.0d0
      tol_gradient = 1.0d-5
      nump = 1
      gpr = new_gaussian_process_regression(nump, num_samples, "ARD_without_noise", &
                  TRIM(fn_training_inputs_1D), TRIM(fn_training_targets_1D), &
                  'default')
     
          call timer_start(tm, "---> Regression xSin(x) without noise")
      res = gpr%maximize_marginal_probability(stepsize, maxiter, &
                                              tol_line, tol_gradient)
          call timer_stop(tm)

          call timer_start(tm, "---> Prediction")
      call gpr%write_gpr_state("gpr_state_1D.dat")
          call timer_stop(tm)

      gprp = new_gaussian_process_regression("gpr_state_1D.dat")
      call gprp%make_prediction(predInputs1D, num, predTargets1D)

      msqe = 0.0d0
      DO i = 1, num
         x    = predTargets1D(i) - predTrueValue1D(i)
         msqe = msqe + x*x
      ENDDO
      msqe = msqe/num

      PRINT*, "    MSQE: ", msqe

      if (my_rank == 0) then
         open(newunit=u,file='prediction_targets_1D.bin',form='unformatted')
         write(u) predTargets1D(1:num)
         close(u)
      endif

      gprn = new_gaussian_process_regression(nump, num_samples, "ARD_with_noise", &
                   fn_training_inputs_1D, fn_training_targets_1D, &
                   'default')
      call gprn%set_signal_to_noise(1.0d+6)
          call timer_start(tm, "---> Regression xSin(x) with noise")
      res = gprn%maximize_marginal_probability(stepsize, maxiter, &
                  tol_line, tol_gradient)
          call timer_stop(tm)

          call timer_start(tm, "---> Prediction")
      call gprn%make_prediction(predInputs1D, num, predTargets1D)
          call timer_stop(tm)

      msqe = 0.0d0
      DO i = 1, num
         x    = predTargets1D(i) - predTrueValue1D(i)
         msqe = msqe + x*x
      ENDDO
      msqe = msqe/num

      PRINT*, "    MSQE: ", msqe

      if (my_rank == 0) then
         open(newunit=u,file='prediction_targets_1D.bin',form='unformatted')
         write(u) predTargets1D
         close(u)
      endif

      DEALLOCATE(predInputs1D)
      DEALLOCATE(predTargets1D)
      DEALLOCATE(predTrueValue1D)

      PRINT*, '--------------------------------------------'
      PRINT*, '---> Performing 2D Regression Test Case <---'
      PRINT*, '--------------------------------------------'

      ALLOCATE(predInputs2D(2*num*num))
      ALLOCATE(predTargets2D(num*num))
      ALLOCATE(predTrueValue2D(num*num))

      DO i = 1, num
         x = h*(i-1)
         DO j = 1, num
                                       y = h*(j-1)
                                       n = 2*(j-1)+2*num*(i-1)
                       predInputs2D(1+n) = x
                       predInputs2D(2+n) = y
            predTrueValue2D(j+num*(i-1)) = x*sin(x) + y*sin(y)
         ENDDO
      ENDDO

      nump = 2
      gpr2D = new_gaussian_process_regression(nump, num_samples*num_samples, "ARD_without_noise", &
                  TRIM(fn_training_inputs_2D), &
                  TRIM(fn_training_targets_2D),'default')
      call gpr2D%set_kernel_nugget(1.0d-3)

          call timer_start(tm, "---> xSin(x)+ySin(y) without noise")
      res = gpr2D%maximize_marginal_probability(stepsize, maxiter, &
                                              tol_line, tol_gradient)
          call timer_stop(tm)

          call timer_start(tm, "---> Prediction")
      call gpr2D%make_prediction(predInputs2D, num*num, predTargets2D)
          call timer_stop(tm)

      msqe = 0.0d0
      DO i = 1, num*num
         x    = predTargets2D(i) - predTrueValue2D(i)
         msqe = msqe + x*x
      ENDDO
      msqe = msqe/(num*num)

      PRINT*, "     MSQE: ", msqe

      if (my_rank == 0) then
         open(newunit=u,file='prediction_targets_2D.bin',form='unformatted')
         write(u) predTargets2D(1:num*num)
         close(u)
      endif

      gpr2Dn = new_gaussian_process_regression(nump, num_samples*num_samples, "ARD_with_noise", &
                  TRIM(fn_training_inputs_2D), &
                  TRIM(fn_training_targets_2D),'default')
      call gpr2Dn%set_signal_to_noise(1.0d+4)
      call gpr2Dn%set_kernel_nugget(1.0d-3)
      
          call timer_start(tm, "---> xSin(x)+ySin(y) with noise")
      res = gpr2Dn%maximize_marginal_probability(stepsize, maxiter, &
                                              tol_line, tol_gradient)
          call timer_stop(tm)
      
          call timer_start(tm, "---> Prediction")
      call gpr2Dn%make_prediction(predInputs2D, num*num, predTargets2D)
          call timer_stop(tm)

      msqe = 0.0d0
      DO i = 1, num*num
         x    = predTargets2D(i) - predTrueValue2D(i)
         msqe = msqe + x*x
      ENDDO
      msqe = msqe/(num*num)

      PRINT*, "     MSQE: ", msqe

      if (my_rank == 0) then
         open(newunit=u,file='prediction_targets_2D.bin',form='unformatted')
         write(u) predTargets2D(1:num*num)
         close(u)
      endif

      DEALLOCATE(predInputs2D)
      DEALLOCATE(predTargets2D)
      DEALLOCATE(predTrueValue2D)

      call delete_gaussian_process_regression(gpr);
      call delete_gaussian_process_regression(gprp);
      call delete_gaussian_process_regression(gprn);
      call delete_gaussian_process_regression(gpr2D);
      call delete_gaussian_process_regression(gpr2Dn);

      ! end mpi
      call end_parallel()

      end program for_regressionTest

!EOC
!------------------------------------------------------------------------------
