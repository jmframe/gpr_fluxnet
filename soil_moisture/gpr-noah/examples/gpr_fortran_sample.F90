program example_gpr_fortran

use iso_c_binding
use gpr_class
use option_class
use cstring_class
use timer_class
use comm

implicit none

!dummy indexes
integer :: i,j,k

!variable to hold read-in options
character(len=256), target :: fn_gpr_state
character(len=256), target :: fn_training_inputs
character(len=256), target :: fn_training_targets
character(len=256), target :: kernel_type
integer :: sample_size, targetsize, dimx, maxiter
real(kind=8) :: stepsize, tol_line, tol_gradient
real(kind=8) :: signal_to_noise, signal_to_kernel, ll

!cstrings for character options
type(cstring) :: c_fn_gpr_state
type(cstring) :: c_fn_training_inputs
type(cstring) :: c_fn_training_targets
type(cstring) :: c_kernel_type

!inputs for perdiction
real(kind=8), allocatable :: p_inputs(:)
real(kind=8), allocatable :: p_targets(:)
real(kind=8), allocatable :: targets(:)

!option class that holds all read-in options
type(options) :: opt

!variables to get argc and argv
type(c_ptr), allocatable, target :: argv(:)
integer :: argc

!gaussian process class (this wraps the c++ class)
type(gaussian_process_regression) :: gpr, pgpr
character(kind=c_char,len=1) :: v

!timer
type(timer) :: tm

!initialize MPI
call get_command_args(argc,argv)
call start_parallel(argc,argv(1))

!initialize cstrings
c_fn_gpr_state        = new_cstring()
c_fn_training_inputs  = new_cstring()
c_fn_training_targets = new_cstring()
c_kernel_type         = new_cstring()

!add optioins --- as many as you want
opt = new_options()
call opt%add('fn_gpr_state', c_fn_gpr_state)
call opt%add('fn_training_inputs', c_fn_training_inputs)
call opt%add('fn_training_targets',c_fn_training_targets)
call opt%add('kernel_type',c_kernel_type)
call opt%add('sample_size',sample_size)
call opt%add('dimx',dimx)
call opt%add('maxiter',maxiter)
call opt%add('stepsize',stepsize)
call opt%add('tol_line',tol_line)
call opt%add('tol_gradient',tol_gradient)
call opt%add('signal_to_noise',signal_to_noise)
call opt%add('signal_to_kernel',signal_to_kernel)
call opt%read_options(argc,argv(1))

!copy cstrings to regular fortran character arrays
call c_fn_gpr_state%get_string(fn_gpr_state)
call c_fn_training_inputs%get_string(fn_training_inputs)
call c_fn_training_targets%get_string(fn_training_targets)
call c_kernel_type%get_string(kernel_type)

!start timer
tm = new_timer()

call tm%start('Begin training')
gpr = new_gaussian_process_regression(dimx,sample_size,kernel_type, &
fn_training_inputs, fn_training_targets,'default')

!set the signal-noise initial gues
call gpr%set_signal_to_noise(signal_to_noise)

!set the signal-kernel nugget
call gpr%set_signal_to_kernel(signal_to_kernel)

!maximize marginal logliklihood to set hyperparameters
ll = gpr%maximize_marginal_probability(stepsize,maxiter,tol_line,tol_gradient)

!stop the timer and print time
call tm%stop()
print *

if(get_rank() == 0) then
write(*,*) 'loglikliehood = ', ll
endif

!save gpr state to file to use later
call gpr%write_gpr_state(fn_gpr_state)

!alloc new gpr from saved state and make prediction at training points
!NOTE: you could have predicted without reading and writing
!this is an example since you will often saved the trained state and predicted later
pgpr = new_gaussian_process_regression(fn_gpr_state)

!read in binary inputs
allocate(p_inputs (1:sample_size))
open(unit=1, file=fn_training_inputs, form='unformatted', access='direct', recl=sample_size*8)
read(1,rec=1) p_inputs
close(unit=1)

!read in binary targets
allocate(targets(1:sample_size))
open(unit=1, file=fn_training_targets, form='unformatted', access='direct', recl=sample_size*8)
read(1,rec=1) targets
close(unit=1)

!make prediction
allocate(p_targets(1:sample_size))
call pgpr%make_prediction(p_inputs,sample_size,p_targets)

print *, 'absolute error = ', sum(abs(p_targets-targets))

!clean up
call delete_options(opt)
call delete_gaussian_process_regression(gpr)
call delete_gaussian_process_regression(pgpr)
call delete_cstring(c_fn_gpr_state)
call delete_cstring(c_fn_training_inputs)
call delete_cstring(c_fn_training_targets)
call delete_cstring(c_kernel_type)

! end mpi
call end_parallel()
end program example_gpr_fortran
