program run_timestep
 use noahmp_veg_parameters
 use noahmp_globals
 use type_decs

 use iso_c_binding
 use gpr_class

! -- Variable Declarations ----------------------------------------------
 implicit none

 ! simulation parameters
 integer :: Nt
 ! Equilibration
 integer :: EQs, osyear, firstObs, initCycle = 1
 ! file I/O
 integer, parameter :: fid = 15
 character(1000)    :: fname, fdir, sname
 character(4) :: charyear

 ! internal indexes
 integer :: s, t, l, d, lags
 real    :: dummy
 integer :: vegtyp
 real    :: rz
 logical :: fixed, fexists 
 integer, allocatable, dimension(:,:) :: date
 real, allocatable, dimension(:) :: time

 ! model data
 type(forcing_data), allocatable, dimension(:) :: forcing
 type(state_data),   allocatable, dimension(:) :: state, background
 type(state_data) :: state_tmp
 type(setup_data) :: setup
 type(setup_data),   allocatable, dimension(:)   :: setup_tmp
 type(output_data),  allocatable, dimension(:) :: output

 ! obs data
 real, allocatable, dimension(:)   :: obs, obs_full, OS

 !CRAIG 
 type(gaussian_process_regression) :: dry_gpr, wet_gpr
 real(kind=8), allocatable :: dinputs(:)
 real(kind=8), allocatable :: winputs(:)
 real(kind=8), allocatable :: correction(:)
 real(kind=8), allocatable, dimension(:) :: correctionz(:)
 real :: random_normal, noise, meanlag, damp_correction
 real(kind=8), dimension(3) :: lagged

 ! Add the abilitty to turn on and off the dynamic correction factor
 logical :: isCorrect
 ! Need to be able to run the onestep simulation here.
 logical :: isOneStep
 character(20) :: onestepper, onestepout, onestepstates
 ! logic to do ONLY one training or the other
 logical :: isTrain, isTrainDry, isTrainWet
 ! equilibration
 logical :: doEQ
! --- Set Up Run --------------------------------------------------------

 character(len=32) :: arg
 call get_command_argument(1, arg)
 if(arg == "0") then
   isOneStep = .false.
 elseif(arg =="1") then
   isOneStep = .true.
   onestepper = "obs.txt"
   onestepout= "output.onestep"
   onestepstates = "states.onestep"
 elseif(arg == "2") then
   isOneStep = .true.
   onestepper = "output.dry" 
   onestepout = "output.drystep"
   onestepstates = "states.drystep"
 endif
 
 call get_command_argument(2, arg)
 if(arg == "0") then
   isCorrect = .false.
 else
   isCorrect = .true.
 endif
 
 call get_command_argument(3, arg)
 if(arg == "0") then
   isTrain = .false.
   isTrainDry = .false.
   isTrainWet = .false.
 elseif(arg == "1") then
   isTrain = .true.
   isTrainDry = .false.
   isTrainWet = .false.
 elseif(arg == "2") then
   isTrain = .true.
   isTrainDry = .true.
   isTrainWet = .false.
 elseif(arg == "3") then
   isTrain = .true.
   isTrainDry = .false.
   isTrainWet = .true.
 endif

 ! number of times to cycle through initialization 
 call get_command_argument(4, arg)
 if(arg == "0") then
  doEQ = .false.
 else
  doEQ = .true.
  READ(arg,*) EQs
 endif

! setup simulation 
 call sim_init(setup,state_tmp,Nt)
 allocate(state(Nt))
 allocate(background(Nt))
 allocate(setup_tmp(1))
 allocate(output(Nt))
 call sim_init(setup_tmp(1),state(1),Nt)
 do t = 2,Nt
   allocate(state(t)%stc(-setup%nsnow+1:setup%nsoil))
   allocate(state(t)%zsnso(-setup%nsnow+1:setup%nsoil))
   allocate(state(t)%tsno(setup%nsnow))
   allocate(state(t)%snice(setup%nsnow))
   allocate(state(t)%snliq(setup%nsnow))
   allocate(state(t)%sh2o(setup%nsoil))
   allocate(state(t)%smc(setup%nsoil))
   allocate(background(t)%stc(-setup%nsnow+1:setup%nsoil))
   allocate(background(t)%zsnso(-setup%nsnow+1:setup%nsoil))
   allocate(background(t)%tsno(setup%nsnow))
   allocate(background(t)%snice(setup%nsnow))
   allocate(background(t)%snliq(setup%nsnow))
   allocate(background(t)%sh2o(setup%nsoil))
   allocate(background(t)%smc(setup%nsoil))
 enddo

 allocate(obs(Nt))
 obs = -9999.
 allocate(obs_full(Nt))
 obs_full = -9999.
 allocate(OS(Nt))
 OS = -9999.
 allocate(correctionz(Nt))
 correctionz = 0.

 ! We want to know the out-of-sample year
 ! to set the soil moisture value to observation, during training.
 open(fid,file='year.txt')
   read(fid,*) osyear
 close(fid)

 ! Take out of the data assimilation logic, because we use it for other things. 
 open(fid,file='obs.txt')
   do t = 1,Nt
     ! jmframe: Make sure matches observation file. Added 1 dummy.
     !Year Day hour   ?     ?     Soil Moisture? Soil Moisture? Precip Rate? 
     !2002 1   0.000 -0.92 -26.23 0.4086666870   0.4216666794   0.0000559161
     read(fid,*) dummy,dummy,dummy,dummy,dummy,obs(t),dummy,dummy
     if ((isOneStep .eqv. .true.) .and. (onestepper .eq. "obs.txt")) then
       OS(t) = obs(t)
     endif
   enddo ! time
   do t = 1,Nt
     if (obs(t).gt.0) then
       firstObs = t
       exit
     endif
   enddo
 close(fid)
 if ((isOneStep .eqv. .true.) .and. (onestepper .ne. "obs.txt")) then
   open(fid,file=onestepper)
     do t = 1,Nt
       read(fid,*) dummy,dummy,dummy,                             & ! Date and time
                   dummy, dummy, dummy, dummy, dummy, dummy,dummy,& ! Forcings 
                   OS(t),dummy,dummy,dummy,                       & !smc
                   dummy,dummy,dummy,dummy,                       & !sh2o
                   dummy, dummy, dummy, dummy, dummy, dummy, dummy  !States and outputs
     enddo ! time
   close(fid)
 endif
 
  ! During training we'll want to set the out-of-sample values to observation.
  ! Do this in order to have the best initial states before the gpr corrections.
  if(isTrain) then
   open(fid,file='obs_full.txt')
     do t = 1,Nt
       read(fid,*) dummy,dummy,dummy,dummy,dummy,obs_full(t),dummy,dummy
     enddo ! time
  close(fid)
 endif

 !!!!!!!!!!!!!!         input number of parameters for wet/dry   !!!!!!!!!
 allocate(winputs(18))
 allocate(dinputs(17))
 allocate(correction(1))
 
 ! Read in and load in the Gausian Process Regression.
 dry_gpr = new_gaussian_process_regression("dry_gpr.dat")
 wet_gpr = new_gaussian_process_regression("wet_gpr.dat")

! forcing from file
 allocate(forcing(Nt))
 allocate(date(Nt,2))
 allocate(time(Nt))
 fdir = './forcing'

 ! Load in the forcing data
 fname = trim(fdir)//'.txt'
 open(fid,file=trim(fname))
 do t = 1,Nt
  ! the humidity here is kg/kg, not % and not relative humidity.
  read(fid,*) date(t,:),time(t),forcing(t)%sfcspd,dummy,   &
              forcing(t)%sfctmp,forcing(t)%q2,                 &
              forcing(t)%sfcprs,forcing(t)%swrad,              &
              forcing(t)%lwrad,forcing(t)%prcprate
 enddo ! times
 close(fid)

 ! prescribed shade fraction
 forcing%shdfac = -9999.
 inquire(file='shdfac.txt',exist=fexists)
 if ((setup%dveg.eq.1).and.(fexists)) then
  fname = 'shdfac.txt'
  open(fid,file=trim(fname))
    do t = 1,Nt
      read(fid,*) dummy,dummy,dummy,dummy,forcing(t)%shdfac
    enddo ! times
  close(fid)
 endif

! parameters
! This standalone version does not use the parameter tables.
! The one place where this may cause a problem is on...
! line 8866 of module_sf_noahmplsm.f90 where carbon partitioning 
! to the leaf is different for elbforest than for other vegetation types. 
! We have set a constant vegetation type so that isurban, 
! iswater, issnow, and isbaren are not triggered.

 call redprm(setup%nsoil,setup%tbot,vegtyp)
 setup%vegtyp = vegtyp ! this should !not! be a parameter 

! --- Run the Model -----------------------------------------------------
! initial timestep
 t = 1

 ! Call the Noah-MP driver for the initial timestep.
 call driver(t,setup,forcing(t),state(t),output(t))

 ! Read in the state data after equilibration 
 if (doEQ .eqv. .false.) then
   call read_state_data(state(1), 'state.init')
 endif

 ! Update the state if this is the first observation
 if (t.eq.firstObs) then
   do d = 1,4
     state(t)%smc(d) = obs(t)
   enddo
   state(t)%sh2o = state(t)%smc
 endif

 ! Set initial lagged values to time 1
 lagged(1)  = state(1)%smc(1)
 lagged(2)  = state(1)%smc(1)
 lagged(3)  = state(1)%smc(1)

 ! time loop
 t=2
 do while (t.le.Nt)

   ! Just need the year to determine the state initialization
   write(charyear,'(I4)') date(t,1)

   !store gpr lagged values before cut
   lagged(3) = lagged(2)
   lagged(2) = lagged(1)
   lagged(1)  = state(t-1)%smc(1)

   ! Bring the full state forward to predict the current state with Driver.
   state(t) = state(t-1)

   ! frozen water in top soil
   dummy = state(t-1)%smc(1) - state(t-1)%sh2o(1) 

   ! DO the onestep update with OS (either obs or noah+gpr output, assigned above)
   if((isOneStep).and.(obs(t-1).gt.0)) then
     state(t)%smc(1) = OS(t-1)
   endif

   ! Cut state values if outside physical limits
   if (state(t)%lfmass.le.50/SLA)     &
     state(t)%lfmass = 50/SLA+0.01
   if (state(t)%lfmass.ge.5000/SLA)   &
     state(t)%lfmass = 5000/SLA
   if (state(t)%stmass.le.0.05/0.003) &
     state(t)%stmass = 0.05/0.003+0.01
   if (state(t)%rtmass.le.5)          &
     state(t)%rtmass = 5.01 
   state(t)%lai = MAX(state(t)%lfmass*SLA/1000,0.05)
   state(t)%sai = MAX(state(t)%stmass*0.003,0.05)
   do d = 1,setup%nsoil
     if (state(t)%smc(d).gt.smcmax)   &
       state(t)%smc(d) = smcmax
     if (state(t)%smc(d).lt.smcdry)     &
       state(t)%smc(d) = smcdry
     if (state(t)%sh2o(d).gt.smcmax)  &
       state(t)%sh2o(d) = smcmax
     if (state(t)%sh2o(d).lt.smcdry)    &
       state(t)%sh2o(d) = smcdry
   enddo ! soil dimension

   ! conserve water content fixed frozen content
   state(t)%sh2o(1) = (state(t)%smc(1) - dummy) 

   ! run model at timestep
   call driver(t,setup,forcing(t),state(t),output(t))

   ! Initialize the first observation value.
   if (t.eq.firstObs) then
     do d = 1,4
       state(t)%smc(d) = obs(t)
     enddo
     state(t)%sh2o = state(t)%smc
   endif

   ! Use Gaussian Process Regression to dynamically,
   ! update the state of the hydrology model.
   if (isCorrect) then
     ! during training set the out-of-sample year to observation
!     if((isTrain).and.(date(t,1).eq.osyear).and.(obs(t).gt.0)) then
!       state(t)%smc(1)  = obs_full(t)
!       state(t)%sh2o(1) = obs_full(t)
!       correction(1) = 0

     ! if there is no good observation available, set to model
     ! e.g., we can't train the correction if there is no observation to correct to.

     ! else if((isTrain).and.(obs(t).le.0)) then
     if((isTrain).and.(obs(t).le.0)) then
       correction(1) = 0

     ! Calculate the GPR correction for the periods with rain (wet)
     else if(forcing(t)%prcprate .gt. 0) then
       winputs(1) = lagged(1)
       winputs(2) = lagged(2)
       winputs(3) = lagged(3)
       winputs(4) = state(t)%smc(1)
       winputs(5) = forcing(t)%q2
       winputs(6) = forcing(t)%prcprate
       winputs(7) = forcing(t)%lwrad
       winputs(8) = forcing(t)%swrad
       winputs(9) = forcing(t)%sfcprs
       winputs(10) = forcing(t)%sfctmp
       winputs(11)= forcing(t)%sfcspd
       winputs(12)= state(t)%smc(2)
       winputs(13)= state(t)%smc(3)
       winputs(14)= state(t)%smc(4)
       winputs(15)= state(t)%sh2o(1)
       winputs(16)= state(t)%sh2o(2)
       winputs(17)= state(t)%sh2o(3)
       winputs(18)= state(t)%sh2o(4)
       if(isTrainDry) then
         correction(1) = obs(t) - state(t)%smc(1) !set perfect correction
       else
         call wet_gpr%make_prediction(winputs,1,correction)
       endif
     else ! Calculate the GPR correction for the periods with NO rain (dry)
       dinputs(1) = lagged(1)
       dinputs(2) = lagged(2)
       dinputs(3) = lagged(3)
       dinputs(4) = state(t)%smc(1)
       dinputs(5) = forcing(t)%q2
       ! Precipitation would go here in wet: winputs(6) = forcing(t)%prcprate
       dinputs(6) = forcing(t)%lwrad
       dinputs(7) = forcing(t)%swrad
       dinputs(8) = forcing(t)%sfcprs
       dinputs(9) = forcing(t)%sfctmp
       dinputs(10)= forcing(t)%sfcspd
       dinputs(11)= state(t)%smc(2)
       dinputs(12)= state(t)%smc(3)
       dinputs(13)= state(t)%smc(4)
       dinputs(14)= state(t)%sh2o(1)
       dinputs(15)= state(t)%sh2o(2)
       dinputs(16)= state(t)%sh2o(3)
       dinputs(17)= state(t)%sh2o(4)
       if(isTrainWet) then
         correction(1) = obs(t) - state(t)%smc(1) !set perfect correction
       else
         call dry_gpr%make_prediction(dinputs,1,correction)
       endif
     endif
     ! jmframe July 2020: add in a factor to reduce the correction by some stochastic ammount each time step
     damp_correction = 1 !random_normal()
     state(t)%smc(1)  = state(t)%smc(1)  + correction(1) * damp_correction
     state(t)%sh2o(1) = state(t)%sh2o(1) + correction(1) * damp_correction !conserve water content fixed frozen content
     correctionz(t) = correction(1)
     correction(1) = 0
   endif


!   ! Initialize the first observation value.
!   ! Any time that observations begin, use the first one.
!   if ((t.eq.firstObs).or.((obs(t-1).lt.0).and.(obs(t).gt.0)))then
!     do d = 1,4
!       state(t)%smc(d) = obs(t)
!     enddo
!     state(t)%sh2o = state(t)%smc
!   endif
!
!   ! Write the state of the first timestep of each year, during Onsestep equilibration
!   ! read the state of the first timestep of each year, during regular runs.
!   if (date(t,1).gt.date(t-1,1)) then
!     if((doEQ).and.(isOneStep)) then
!       call write_state_data(state(t), 'state.'//charyear)
!     else
!       call read_state_data(state(t), 'state.'//charyear)
!     endif
!   endif

   ! Eqlibration process (i.e., update time 1 with the value at time end.)
   if (doEQ) then
     if (t.eq.Nt) then
       if (initCycle.le.EQs) then
         t=1
         initCycle = initCycle + 1
         state(t) = state(Nt)
         print*, 'Initialization cycle =', initCycle-1
       endif
       if (initCycle.le.EQs) then
         call rmse_obj_fun(date,time,state(:),output(:),Nt)
       endif
     endif
   endif
   t = t + 1
 enddo ! time loop
 
 !CRAIG --- write final state
 if(doEQ) call write_state_data(state(Nt), 'state.init')

! ------- Write Output ------------------------------------------------
 ! Write the main output file reguardless of the other options.
 if (isCorrect) then
   if (isTrainDry) then
     fname = 'output.dry'
     sname = 'states.dry'
   elseif (isTrainWet) then
     fname = 'output.wet'
     sname = 'states.wet'
   else
     fname = 'output.gpr'
     sname = 'states.gpr'
   endif
 elseif (isOneStep) then
   fname = onestepout
   sname = onestepstates
 else
   fname = 'output.noah'
   sname = 'states.noah'
 endif
 
 open(fid,file=trim(fname),status='replace')
 do t = 1,Nt
  write(fid,'( i7,i5,f7.3,                                          &
             f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6        & 
             f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6        & 
             f17.6,f17.6,f17.6,f17.6                                &
             f17.6,f17.6,f17.6,f17.6,f17.6,f17.6                    &
             f17.6)')                                               & 
           date(t,:),time(t),                                       &
           forcing(t)%sfctmp, forcing(t)%sfcspd, forcing(t)%sfcprs, &
           forcing(t)%q2, forcing(t)%lwrad, forcing(t)%swrad,       &
           forcing(t)%prcprate, state(t)%smc(1:4),                  &
           state(t)%sh2o(1:4),                                      &
           state(t)%rtmass, state(t)%wood,                          &
           state(t)%lfmass, state(t)%stmass,                        &
           output(t)%qe, output(t)%qh, output(t)%nee,               &
           correctionz(t)
 enddo
 close(fid)
 ! Printing out all the states to analyze the GPR correction 
 open(fid,file=trim(sname),status='replace')
 do t = 1,Nt
  write(fid,'(i7,i5,f7.3,                                   &
             f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
             f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6& 
             f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
             f17.6,f17.6,f17.6,f17.6,f17.6,f17.6            & 
             f17.6,f17.6,f17.6,f17.6                        &
             f17.6,f17.6,f17.6,f17.6                        &
             f17.6,f17.6,f17.6,f17.6                        &
             f17.6,f17.6,f17.6,f17.6                        &
             f17.6,f17.6,f17.6,f17.6                        &
             f17.6,f17.6,f17.6,f17.6                        &
             f17.6,f17.6,f17.6,f17.6)')                     & 
           date(t,:),time(t), &
           state(t)%albold, state(t)%sneqvo, state(t)%tah, state(t)%eah, state(t)%fwet,  state(t)%canliq, state(t)%canice, &
           state(t)%tv, state(t)%tg, state(t)%qsnow, state(t)%snowh, state(t)%sneqv, state(t)%zwt, state(t)%wa, state(t)%wt, &
           state(t)%wslake, state(t)%lfmass, state(t)%rtmass, state(t)%stmass, state(t)%wood, state(t)%stblcp, state(t)%fastcp, &
           state(t)%lai, state(t)%sai, state(t)%cm, state(t)%ch, state(t)%tauss, state(t)%smcwtd, &
           state(t)%stc(1:4), &
           state(t)%zsnso(1:4), &
           state(t)%sh2o(1:4), &
           state(t)%smc(1:4), &
           state(t)%tsno(1:4), &
           state(t)%snice(1:4), &
           state(t)%snliq(1:4)
 enddo
 close(fid)

 call rmse_obj_fun(date,time,state(:),output(:),Nt)
 print*, 'The program has finished'
! -----------------------------------------------------------------------
end program

! ----------------------------------------------------------------
! ----------------------------------------------------------------
subroutine redprm(nsoil,tbot,vegtyp)
 use noahmp_globals
 use noahmp_veg_parameters
 implicit none

 ! inputs 
 integer, intent(in)    :: nsoil

 ! Locals
 integer, parameter :: fid = 134
 real    :: refdk
 real    :: refkdt
 real    :: frzk
 real    :: frzfact
 real    :: tbot
 character(1000) :: fname
! character(1) :: es1
! character(2) :: es2
 integer :: vegtyp

! ----Read in Paramter File----------------------------------------------

 fname = 'cal_parms.txt'
 open(fid,file=trim(fname),action='read')
  read(fid,*) Z0MVT
  read(fid,*) HVT
  read(fid,*) HVB
  read(fid,*) LTOVRC
  read(fid,*) DILEFW
  read(fid,*) RMF25
  read(fid,*) SLA
  read(fid,*) VCMX25
  read(fid,*) QE25
  read(fid,*) BEXP
  read(fid,*) DKSAT
  ! jmframe June 2019: taking out SMCDRY from the calibration,
  ! Since it is hard coded in for data assimilation
  !  read(fid,*) SMCDRY
  read(fid,*) SMCMAX
  read(fid,*) SMCREF
  read(fid,*) SMCWLT
 close(fid)

 ! turn conductivity into real-valued space
 DKSAT = 10**DKSAT

 ! jmframe June 2019: Moved fractional soil moisture calcs below... 
 ! the rest of the parm inputs.

 ! ensure that canopy top is not lower than canopy bottom
 !HVB = 0.1 + HVB*(HVT-0.1)
 HVB = HVB*HVT

 fname = 'parms.txt'
 open(fid,file=trim(fname),action='read')

 ! veg parms
  read(fid,*) CH2OP
  read(fid,*) DLEAF
  !read(fid,*) Z0MVT
  !read(fid,*) HVT
  !read(fid,*) HVB
  read(fid,*) RC
  read(fid,*) RHOL(1)
  read(fid,*) RHOL(2)
  read(fid,*) RHOS(1)
  read(fid,*) RHOS(2)
  read(fid,*) TAUL(1)
  read(fid,*) TAUL(2)
  read(fid,*) TAUS(1)
  read(fid,*) TAUS(2)
  read(fid,*) XL
  read(fid,*) CWPVT
  read(fid,*) C3PSN
  read(fid,*) KC25
  read(fid,*) AKC
  read(fid,*) KO25
  read(fid,*) AKO
  read(fid,*) AVCMX
  !read(fid,*) LTOVRC
  read(fid,*) DILEFC
  !read(fid,*) DILEFW
  !read(fid,*) RMF25
  !read(fid,*) SLA
  read(fid,*) FRAGR
  read(fid,*) TMIN
  !read(fid,*) VCMX25
  read(fid,*) TDLEF
  read(fid,*) BP
  read(fid,*) MP
  !read(fid,*) QE25
  read(fid,*) RMS25
  read(fid,*) RMR25
  read(fid,*) ARM
  read(fid,*) FOLNMX
  read(fid,*) WDPOOL
  read(fid,*) WRRAT
  read(fid,*) MRP
  SAIM = 0.
  LAIM = 0.
  !read(fid,*) SAIM
  !read(fid,*) LAIM
  read(fid,*) SLAREA
  !read(fid,*) EPS
  read(fid,*) VEGTYP

 ! gen parms
  read(fid,*) csoil
  !read(fid,*) bexp
  !read(fid,*) dksat
  read(fid,*) dwsat
  read(fid,*) f1
  read(fid,*) psisat
  read(fid,*) quartz
  !jmframe: Not calibrating. Hardwired in as 0.02
  ! Added into parms.txt file as line 40
  read(fid,*) smcdry
  !read(fid,*) smcmax
  !read(fid,*) smcref
  !read(fid,*) smcwlt

  read(fid,*) zbot      ! 55
  read(fid,*) czil
  read(fid,*) frzk
  read(fid,*) refdk
  read(fid,*) refkdt
  read(fid,*) slope     ! 60
  read(fid,*) topt      ! 61
  read(fid,*) rgl       ! 62
  read(fid,*) rsmax     ! 63
  read(fid,*) rsmin     ! 64
  read(fid,*) hs        ! 65
  read(fid,*) nroot     
 close(fid)

 open(fid,file='tbot.txt',action='read')
  read(fid,*) tbot ! 67
 close(fid)

 open(fid,file='time_parms.txt',action='read')
  read(fid,*) LAIM   
  read(fid,*) SAIM    
!  read(fid,*) EPS
 close(fid)

 ! jmframe June 2019: moved from above, to take SMCDRY out of calibration.
 ! expand soil parameters as fractional intervals
 smcmax = smcdry + smcmax
 smcref = smcdry + (smcmax-smcdry)*smcref
 smcwlt = smcdry + (smcref-smcdry)*smcwlt

 ! some basic manipulations
 kdt = refkdt * dksat / refdk
 frzfact = (smcmax / smcref) * (0.412 / 0.468)
 frzx = frzk * frzfact

 ! error check on rooting layers
 if (nroot.gt.nsoil) nroot = nsoil

end subroutine redprm

subroutine sim_init(setup,state,Ntimes)
 use type_decs

 integer, parameter :: fid = 14
 type(state_data)   :: state
 type(setup_data)   :: setup
 integer, intent(out) :: Ntimes
 logical :: fexists

! simulation setup
 open(fid,file='init.txt',action='read')
  read(fid,*) setup%nsoil     
  read(fid,*) setup%nsnow     
 
! allocate dimensions
  allocate(state%stc(-setup%nsnow+1:setup%nsoil))
  allocate(state%zsnso(-setup%nsnow+1:setup%nsoil))
  allocate(state%tsno(setup%nsnow))
  allocate(state%snice(setup%nsnow))
  allocate(state%snliq(setup%nsnow))
  allocate(state%sh2o(setup%nsoil))
  allocate(state%smc(setup%nsoil))
  allocate(setup%sldpth(setup%nsoil))

! setup parameters
  read(fid,*) setup%zlvl      
  read(fid,*) setup%dt        
  read(fid,*) setup%opt_crs 
  read(fid,*) setup%opt_btr 
  read(fid,*) setup%opt_run 
  read(fid,*) setup%opt_sfc 
  read(fid,*) setup%opt_frz 
  read(fid,*) setup%opt_inf 
  read(fid,*) setup%opt_rad 
  read(fid,*) setup%opt_alb 
  read(fid,*) setup%opt_snf 
  read(fid,*) setup%opt_tbot 
  read(fid,*) setup%opt_stc 
  read(fid,*) setup%dveg     
  read(fid,*) setup%sldpth    

! initial state
  read(fid,*) state%stc(1:setup%nsoil) 
  read(fid,*) state%snowh   
  read(fid,*) state%sneqv   
  read(fid,*) state%canliq  
  read(fid,*) state%rtmass  
  read(fid,*) state%albold  
  read(fid,*) state%lai     
  read(fid,*) state%tv      
  read(fid,*) state%tg      
 close(fid)

 open(fid,file='startdate.txt')
  read(fid,*) setup%startdate
 close(fid)

 open(fid,file='num_times.txt')
  read(fid,*) ntimes
 close(fid)

 open(fid,file='lat_lon.txt')
  read(fid,*) setup%latitude
  read(fid,*) setup%longitude
 close(fid)

 open(fid,file='plant_init.txt')
  read(fid,*) state%rtmass
  read(fid,*) state%wood
  read(fid,*) state%lfmass
  read(fid,*) state%stmass
 close(fid)

 open(fid,file='soil_init.txt')
  read(fid,*) state%smc(1)
  read(fid,*) state%smc(2)
  read(fid,*) state%smc(3)
  read(fid,*) state%smc(4)
  state%sh2o = state%smc
 close(fid)

 inquire(file='shdfac.txt',exist=fexists)
 if (fexists) then
   open(fid,file='shdfac.txt')
     read(fid,*) setup%shdfac_monthly
   close(fid)
 else
  setup%shdfac_monthly = (/0.98,0.98,0.98,0.98,0.98,0.98,0.98,0.98,0.98,0.98,0.98,0.98/)
 endif

end subroutine sim_init

subroutine dealoc(setup,state)
 use type_decs

 type(state_data)   :: state
 type(setup_data)   :: setup

 deallocate(state%stc)
 deallocate(state%zsnso)
 deallocate(state%tsno)
 deallocate(state%snice)
 deallocate(state%snliq)
 deallocate(state%sh2o)
 deallocate(state%smc)
 deallocate(setup%sldpth)

end subroutine dealoc

function random_normal() result(fn_val)
 implicit none
 real     :: fn_val
 real     :: half = 0.5
 real     :: s = 0.449871, t = -0.386595, a = 0.19600, b = 0.25472,    &
             r1 = 0.27597, r2 = 0.27846, u, v, x, y, q
 integer :: i, n, clock

 do
   call random_number(u)
   call random_number(v)
   v = 1.7156 * (v - half)

   x = u - s
   y = ABS(v) - t
   q = x**2 + y*(a*y - b*x)

   ! Accept P if inside inner ellipse
   if (q < r1) EXIT
   ! Reject P if outside outer ellipse
   if (q > r2) CYCLE
   ! Reject P if outside acceptance region
   if (v**2 < -4.0*LOG(u)*u**2) EXIT
 enddo

 ! Return ratio of P's coordinates as the normal deviate
 fn_val = v/u

 return
end function random_normal

! Copied from Craig's ObjFun.F90 file November 4th.
subroutine write_state_data(state, fname)
 use type_decs
 implicit none

 type(state_data), intent(in)  :: state
 character(len=10), intent(in) :: fname
 integer, parameter :: fid = 14
 integer, parameter :: nsnow = 3
 integer, parameter :: nsoil = 4
 integer :: i 
 
 open(fid,file=fname, status='replace', action='write')
    write(fid,'( f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6)')               & 
                 state%albold, &
                 state%sneqvo,  &
                 state%tah,  &
                 state%eah,  &
                 state%fwet,  &
                 state%canliq,  &
                 state%canice,  &
                 state%tv,    & 
                 state%tg,  &
                 state%qsnow,  &
                 state%snowh,  &
                 state%sneqv,  &
                 state%zwt,  &
                 state%wa,  &
                 state%wt,  &
                 state%wslake,  &
                 state%lfmass, &
                 state%rtmass,  &
                 state%stmass,  &
                 state%wood,  &
                 state%stblcp,  &
                 state%fastcp,  &
                 state%lai,  &
                 state%sai,  &
                 state%cm,    &
                 state%ch,  &
                 state%tauss,  &
                 state%smcwtd
               !  state%deeprech,  &
               !  state%rech  
    write(fid,'(i2)') state%isnow
 
 do i=-nsnow+1,nsoil
   write(fid,'(f17.6)') state%stc(i)
 enddo
 
 do i=-nsnow+1,nsoil
   write(fid,'(f17.6)') state%zsnso(i)
 enddo

 do i=1, nsoil
   write(fid,'(f17.6)') state%sh2o(i)
 enddo
 
 do i=1, nsoil
   write(fid,'(f17.6)') state%smc(i)
 enddo
 
 do i=1,nsnow
   write(fid,'(f17.6)') state%tsno(i)
 enddo

 do i=1,nsnow
   write(fid,'(f17.6)') state%snice(i)
 enddo

 do i=1,nsnow
   write(fid,'(f17.6)') state%snliq(i)
 enddo
  close(fid)
end subroutine

subroutine read_state_data(state, fname)
 use type_decs
 implicit none

 type(state_data), intent(inout)  :: state
 character(len=10), intent(in) :: fname
 integer, parameter :: fid = 14
 integer, parameter :: nsnow = 3
 integer, parameter :: nsoil = 4
 integer :: i 
 logical :: fexists
 
 inquire(file=fname,exist=fexists)
 if (fexists) then
   open(fid,file=fname)
     read(fid,*)               &
                   state%albold, &
                   state%sneqvo,  &
                   state%tah,  &
                   state%eah,  &
                   state%fwet,  &
                   state%canliq,  &
                   state%canice,  &
                   state%tv,    & 
                   state%tg,  &
                   state%qsnow,  &
                   state%snowh,  &
                   state%sneqv,  &
                   state%zwt,  &
                   state%wa,  &
                   state%wt,  &
                   state%wslake,  &
                   state%lfmass, &
                   state%rtmass,  &
                   state%stmass,  &
                   state%wood,  &
                   state%stblcp,  &
                   state%fastcp,  &
                   state%lai,  &
                   state%sai,  &
                   state%cm,    &
                   state%ch,  &
                   state%tauss,  &
                   state%smcwtd
                !   state%deeprech,  &
                !   state%rech  
     read(fid,*) state%isnow

   do i=-nsnow+1,nsoil
     read(fid,*) state%stc(i)
   enddo
   
   do i=-nsnow+1,nsoil
     read(fid,*) state%zsnso(i)
   enddo
   
   do i=1, nsoil
     read(fid,*) state%sh2o(i)
   enddo
   
   do i=1, nsoil
     read(fid,*) state%smc(i)
   enddo
   
   do i=1,nsnow
     read(fid,*) state%tsno(i)
   enddo
   
   do i=1,nsnow
     read(fid,*) state%snice(i)
   enddo
   
   do i=1,nsnow
     read(fid,*) state%snliq(i)
   enddo
     close(fid)
 else
   print*, "State file (",fname,") does not exist. Keeping model state"
 endif
end subroutine
