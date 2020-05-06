Program run_timestep
 use noahmp_veg_parameters
 use noahmp_globals
 use type_decs

 use iso_c_binding
 use gpr_class

! -- Variable Declarations ----------------------------------------------
 implicit none

 ! output parameters
 logical, parameter :: write_model_output = .true.
 logical, parameter :: calc_obj_fun = .true.

 ! simulation parameters
 integer :: Ne, Nt
 logical :: perturb, data_assim

 ! file I/O
 integer, parameter :: fid = 15
 character(1000)    :: fname, fdir
 character(1)       :: es1
 character(2)       :: es2
 character(3)       :: es3

 ! internal indexes
 integer :: s, t, e, ee, l, d, lags
 real    :: dummy
 integer :: vegtyp
 real    :: rz
 logical :: fixed, fexists 
 integer, allocatable, dimension(:,:) :: date
 real, allocatable, dimension(:) :: time

 ! model data
 type(forcing_data), allocatable, dimension(:,:) :: forcing
 type(state_data),   allocatable, dimension(:,:) :: state, background
 type(state_data) :: state_tmp
 type(setup_data) :: setup
 type(setup_data),   allocatable, dimension(:)   :: setup_tmp
 type(output_data),  allocatable, dimension(:,:) :: output

 ! obs data
 integer, parameter                  :: Nlag = 1
 integer, parameter                  :: Dz = 1
 real, allocatable, dimension(:,:)   :: obs
 real, dimension(Dz)                 :: zcov
 real, allocatable, dimension(:,:,:) :: X
 real, allocatable, dimension(:,:)   :: Y
 real, allocatable, dimension(:)     :: Z, R
 
!CRAIG 
 real, allocatable, dimension(:)     :: tower
 type(gaussian_process_regression) :: dry_gpr, wet_gpr
 real(kind=8), allocatable :: dinputs(:)
 real(kind=8), allocatable :: winputs(:)
 real(kind=8), allocatable :: correction(:)
 real :: random_normal, lagged(3), noise
 logical :: isOneStep
 logical :: isCorrect
 logical :: isTrainDry
 logical :: isTrainWet

 ! mean output data
 real ::  mean_sfctmp,mean_sfcspd,mean_sfcprs,mean_q2,mean_lwrad, &
 mean_swrad,mean_prcprate,mean_smc1,mean_smc2,mean_smc3,mean_smc4,&
 mean_wood,mean_lfmass,mean_stmass,mean_rtmass,mean_qe,mean_qh,mean_nee      

! --- Set Up Run --------------------------------------------------------
  character(len=32) :: arg
  call get_command_argument(1, arg)
  if(arg == "0") then
    isOneStep = .false.
  else
    isOneStep = .true.
  endif
  
  call get_command_argument(2, arg)
  if(arg == "0") then
    isCorrect = .false.
  else
    isCorrect = .true.
  endif
  
  call get_command_argument(3, arg)
  if(arg == "0") then
    isTrainDry = .false.
  else
    isTrainDry = .true.
  endif
  
  call get_command_argument(4, arg)
  if(arg == "0") then
    isTrainWet = .false.
  else
    isTrainWet = .true.
  endif
  
  call get_command_argument(5, arg)
  READ(arg,*)noise


! setup simulation 
 call sim_init(setup,state_tmp,perturb,data_assim,Nt,Ne)
 allocate(state(Nt,Ne))
 allocate(background(Nt,Ne))
 allocate(setup_tmp(Ne))
 allocate(output(Nt,Ne))
 do e = 1,Ne
   call sim_init(setup_tmp(e),state(1,e),perturb,data_assim,Nt,Ne)
   do t = 2,Nt
     allocate(state(t,e)%stc(-setup%nsnow+1:setup%nsoil))
     allocate(state(t,e)%zsnso(-setup%nsnow+1:setup%nsoil))
     allocate(state(t,e)%tsno(setup%nsnow))
     allocate(state(t,e)%snice(setup%nsnow))
     allocate(state(t,e)%snliq(setup%nsnow))
     allocate(state(t,e)%sh2o(setup%nsoil))
     allocate(state(t,e)%smc(setup%nsoil))
     allocate(background(t,e)%stc(-setup%nsnow+1:setup%nsoil))
     allocate(background(t,e)%zsnso(-setup%nsnow+1:setup%nsoil))
     allocate(background(t,e)%tsno(setup%nsnow))
     allocate(background(t,e)%snice(setup%nsnow))
     allocate(background(t,e)%snliq(setup%nsnow))
     allocate(background(t,e)%sh2o(setup%nsoil))
     allocate(background(t,e)%smc(setup%nsoil))
   enddo
 enddo

!CRAIG -- read tower observations
 allocate(dinputs(10))
 allocate(winputs(11))
 allocate(correction(1))
 allocate(tower(Nt))
 if(isCorrect) then
 dry_gpr = new_gaussian_process_regression("dry_gpr.dat")
 wet_gpr = new_gaussian_process_regression("wet_gpr.dat")
 endif
 open(fid,file="tower")
 do t = 1,Nt
   read(fid,*) tower(t)
 enddo ! times
 close(fid)

! forcing from file
 allocate(forcing(Nt,Ne))
 allocate(date(Nt,2))
 allocate(time(Nt))
 fdir = './forcing'

 if (Ne.gt.1) then
   do e = 1,Ne
     fname = trim(fdir)//'_'
     if (e.lt.10) then
      write(es1,'(i1)') e
      fname = trim(fname)//es1
     elseif (e.lt.100) then
      write(es2,'(i2)') e
      fname = trim(fname)//es2
     elseif (e.lt.1000) then
      write(es3,'(i3)') e
      fname = trim(fname)//es3
     endif 
     fname = trim(fname)//'.txt'
 
     open(fid,file=trim(fname))
     do t = 1,Nt
       ! the humidity here is kg/kg, not % and not relative humidity.
       read(fid,*) date(t,:),time(t),forcing(t,e)%sfcspd,dummy,   &
                   forcing(t,e)%sfctmp,forcing(t,e)%q2,                 &
                   forcing(t,e)%sfcprs,forcing(t,e)%swrad,              &
                   forcing(t,e)%lwrad,forcing(t,e)%prcprate
     enddo ! times
     close(fid)
   enddo ! ensembles

 else ! only one ensemble member

   e = 1
   fname = trim(fdir)//'.txt'
   open(fid,file=trim(fname))
   do t = 1,Nt
     ! the humidity here is kg/kg, not % and not relative humidity.
     read(fid,*) date(t,:),time(t),forcing(t,e)%sfcspd,dummy,   &
                 forcing(t,e)%sfctmp,forcing(t,e)%q2,                 &
                 forcing(t,e)%sfcprs,forcing(t,e)%swrad,              &
                 forcing(t,e)%lwrad,forcing(t,e)%prcprate
   enddo ! times
   close(fid)
 endif

! prescribed shade fraction
 forcing%shdfac = -9999.
 inquire(file='shdfac.txt',exist=fexists)
 if ((setup%dveg.eq.1).and.(fexists)) then
   do e = 1,Ne
    fname = 'shdfac.txt'
    open(fid,file=trim(fname))
      do t = 1,Nt
        read(fid,*) dummy,dummy,dummy,dummy,forcing(t,e)%shdfac
      enddo ! times
    close(fid)
   enddo ! ensembles
 endif
 
! parameters
! This standalone version does not use the parameter tables. The one place where this may cause a problem is on line 8866 of module_sf_noahmplsm.f90 where carbon partitioning to the leaf is different for elbforest than for other vegetation types. We have set a constant vegetation type so that isurban, iswater, issnow, and isbaren are not triggered.

 do e = 1,Ne
  call redprm(setup%nsoil,setup%tbot,vegtyp)
  setup%vegtyp = vegtyp ! this should !not! be a parameter 
 enddo

! --- Load Observations -------------------------------------------------
! jmframe, May 2019. Load in the observed data anyway, to write a output with
! soil moisture state.
! open(fid,file='obs.txt')
!   do t = 1,Nt
!     read(fid,*) dummy,dummy,dummy,dummy,obs(t,:)
!   enddo ! time
! close(fid)

 if (data_assim) then

   open(fid,file='obs.txt')
     do t = 1,Nt
       read(fid,*) dummy,dummy,dummy,dummy,obs(t,:)
     enddo ! time
   close(fid)

   allocate(obs(Nt,Dz))
   obs = -9999.
  
   fname = 'obs_cov.txt'
   open(fid,file=trim(fname))
    read(fid,*) zcov
   close(fid)
 
 endif

 !CRAIG -- write forcings in correct order to file
   open(fid,file="craig_forcing.txt")
   do t = 1,Nt
     ! the humidity here is kg/kg, not % and not relative humidity.
     write(fid,*) forcing(t,:)
   enddo ! times
   close(fid)

   if(isOneStep) then
     open(fid,file='noise')
     do t = 1,Nt
       !if(forcing(t,1)%prcprate .eq. 0) then
       tower(t) = tower(t)
       !tower(t) = tower(t) + SQRT(noise)*random_normal()
       !endif
       write(fid,*) tower(t)
     enddo ! times
     close(fid)
   endif
!END CRAIG



! --- Run the Model -----------------------------------------------------
! initial timestep
 t = 1
 do e = 1,Ne
   call driver(t,setup,forcing(t,e),state(t,e),output(t,e))
   if (data_assim) then
     background(t,e) = state(t,e)
   endif 
 enddo
 call read_state_data(state(1,1))
 state(1,1)%smc(1) = tower(1)

lagged(1)  = state(1,1)%smc(1) !store gpr prediction before cut
lagged(2)  = state(1,1)%smc(1) !store gpr prediction before cut
lagged(3)  = state(1,1)%smc(1) !store gpr prediction before cut
! time loop
 do t = 2,Nt
! print*, t
   do e = 1,Ne
     !store gpr lagged values before cut
     lagged(1) = lagged(2)
     lagged(2) = lagged(3)
     lagged(3)  = state(t-1,e)%smc(1)

     ! timestep
     state(t,e) = state(t-1,e)
 
     if(isOneStep) then
       state(t,e)%smc(1) = tower(t-1)
       !state(t,e)%sh2o(1) = state(t,e)%sh2o(1) + (state(t,e)%smc(1) - state(t-1,e)%smc(1)) !conserve water content
       !print *, (state(t,e)%smc(1) - state(t-1,e)%smc(1))
     endif

     if (state(t,e)%lfmass.le.50/SLA)     &
       state(t,e)%lfmass = 50/SLA+0.01
     if (state(t,e)%lfmass.ge.5000/SLA)   &
       state(t,e)%lfmass = 5000/SLA
     if (state(t,e)%stmass.le.0.05/0.003) &
       state(t,e)%stmass = 0.05/0.003+0.01
     if (state(t,e)%rtmass.le.5)          &
       state(t,e)%rtmass = 5.01 
     state(t,e)%lai = MAX(state(t,e)%lfmass*SLA/1000,0.05)
     state(t,e)%sai = MAX(state(t,e)%stmass*0.003,0.05)
     do d = 1,setup%nsoil
       if (state(t,e)%smc(d).gt.smcmax)   &
         state(t,e)%smc(d) = smcmax
       if (state(t,e)%smc(d).lt.0.02)     &
         state(t,e)%smc(d) = 0.02
       if (state(t,e)%sh2o(d).gt.smcmax)  &
         state(t,e)%sh2o(d) = smcmax
       if (state(t,e)%sh2o(d).lt.0.02)    &
         state(t,e)%sh2o(d) = 0.02
     enddo ! soil dimension
       
     ! add random perturbation
     if (perturb) then
       call perturb_state(setup,state(t,e))
     endif
     

     ! run model at timestep
     call driver(t,setup,forcing(t,e),state(t,e),output(t,e))
     if (data_assim) then
       background(t,e) = state(t,e)
     endif 

     ! error check in dynamic veg state 
     if (isnan(state(t,e)%stmass).or.isnan(state(t,e)%lfmass)) then
       print*, 'fixing ens member',e,'at time',t
       fixed = .false.
       do ee = e-1,1,-1
         if ((.not.(fixed)).and.(.not.(isnan(state(t,ee)%stmass))) &
              .and.(.not.(isnan(state(t,ee)%lfmass)))) then
           state(t,e) = state(t,ee)
           output(t,e) = output(t,ee)
           background(t,e) = background(t,ee)
           fixed = .true.
         endif
       enddo
       if (.not.(fixed)) stop 'Error in Noah-MP veg state'
     endif

   enddo ! ensemble

   if(isCorrect) then
     !CRAIG -- do correction
     if(forcing(t,1)%prcprate .gt. 0) then
       winputs(1) = lagged(1)
       winputs(2) = lagged(2)
       winputs(3) = lagged(3)
       winputs(4) = state(t,1)%smc(1)
       winputs(5) = forcing(t,1)%q2
       winputs(6) = forcing(t,1)%prcprate
       winputs(7) = forcing(t,1)%lwrad
       winputs(8) = forcing(t,1)%swrad
       winputs(9) = forcing(t,1)%sfcprs
       winputs(10) = forcing(t,1)%sfctmp
       winputs(11)= forcing(t,1)%sfcspd
       call wet_gpr%make_prediction(winputs,1,correction)
     else
       !dinputs(1) = state(t-1,1)%smc(1)
       dinputs(1) = lagged(1)
       dinputs(2) = lagged(2)
       dinputs(3) = lagged(3)
       dinputs(4) = state(t,1)%smc(1)
       dinputs(5) = forcing(t,1)%q2
       dinputs(6) = forcing(t,1)%lwrad
       dinputs(7) = forcing(t,1)%swrad
       dinputs(8) = forcing(t,1)%sfcprs
       dinputs(9) = forcing(t,1)%sfctmp
       dinputs(10)= forcing(t,1)%sfcspd
       call dry_gpr%make_prediction(dinputs,1,correction)
     endif
     state(t,1)%smc(1)  = state(t,1)%smc(1)  + correction(1)
     !state(t,1)%sh2o(1) = state(t,1)%sh2o(1) + correction(1) !conserve water content

     if(isTrainDry) then
       if(forcing(t,1)%prcprate .gt. 0) then
         state(t,1)%smc(1) = tower(t)
       endif
     endif
     
     if(isTrainWet) then
       if(forcing(t,1)%prcprate .eq. 0) then
         state(t,1)%smc(1) = tower(t)
       endif
     endif
     !print *, state(t,1)%smc(1), correction
   endif
   
!   if(forcing(t,1)%prcprate .gt. 0) then
!     state(t,1)%smc(1) = state(t,1)%smc(1)-0.1*(state(t,1)%smc(1)-tower(t))
!   endif
 
 enddo ! time loop
 
 !CRAIG --- write final state
 if(isOneStep) call write_state_data(state(Nt,1))
 
! ------- Write Output ------------------------------------------------
 if (write_model_output) then

  if (perturb) then
   if (data_assim) then
    fname = 'enks_mean.out'
   else
    fname = 'open_mean.out'
   endif
   open(fid,file=trim(fname),status='replace')

   do t = 1,Nt 
    mean_sfctmp   = sum(forcing(t,:)%sfctmp)   /Ne
    mean_sfcspd   = sum(forcing(t,:)%sfcspd)   /Ne
    mean_sfcprs   = sum(forcing(t,:)%sfcprs)   /Ne
    mean_q2       = sum(forcing(t,:)%q2)       /Ne
    mean_lwrad    = sum(forcing(t,:)%lwrad)    /Ne
    mean_swrad    = sum(forcing(t,:)%swrad)    /Ne
    mean_prcprate = sum(forcing(t,:)%prcprate) /Ne

    mean_smc1     = 0
    mean_smc2     = 0
    mean_smc3     = 0
    mean_smc4     = 0
    do e = 1,Ne
     mean_smc1     = mean_smc1 + state(t,e)%smc(1)
     mean_smc2     = mean_smc1 + state(t,e)%smc(2)
     mean_smc3     = mean_smc1 + state(t,e)%smc(3)
     mean_smc4     = mean_smc1 + state(t,e)%smc(4)
    enddo
    mean_smc1     = mean_smc1/Ne
    mean_smc2     = mean_smc2/Ne
    mean_smc3     = mean_smc3/Ne
    mean_smc4     = mean_smc4/Ne

    mean_wood     = sum(state(t,:)%wood)       /Ne
    mean_lfmass   = sum(state(t,:)%lfmass)     /Ne
    mean_stmass   = sum(state(t,:)%stmass)     /Ne
    mean_rtmass   = sum(state(t,:)%rtmass)     /Ne

    mean_qe       = sum(output(t,:)%qe)        /Ne
    mean_qh       = sum(output(t,:)%qh)        /Ne
    mean_nee      = sum(output(t,:)%nee)       /Ne
    write(fid,'(                                 &
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6)')               & 
     mean_sfctmp,mean_sfcspd,mean_sfcprs,mean_q2,mean_lwrad,          &
     mean_swrad,mean_prcprate,                                        &
     mean_smc1,mean_smc2,mean_smc3,mean_smc4,                         &
     mean_rtmass,mean_wood,mean_lfmass,mean_stmass,                   &
     mean_qe,mean_qh,mean_nee
   enddo
   close(fid)
  endif

  if (data_assim) then
   fname = 'back_mean.out'
   open(fid,file=trim(fname),status='replace')
  
   do t = 1,Nt 
    mean_sfctmp   = sum(forcing(t,:)%sfctmp)   /Ne
    mean_sfcspd   = sum(forcing(t,:)%sfcspd)   /Ne
    mean_sfcprs   = sum(forcing(t,:)%sfcprs)   /Ne
    mean_q2       = sum(forcing(t,:)%q2)       /Ne
    mean_lwrad    = sum(forcing(t,:)%lwrad)    /Ne
    mean_swrad    = sum(forcing(t,:)%swrad)    /Ne
    mean_prcprate = sum(forcing(t,:)%prcprate) /Ne
    
    mean_smc1     = 0
    mean_smc2     = 0
    mean_smc3     = 0
    mean_smc4     = 0
    do e = 1,Ne
     mean_smc1     = mean_smc1 + background(t,e)%smc(1)
     mean_smc2     = mean_smc1 + background(t,e)%smc(2)
     mean_smc3     = mean_smc1 + background(t,e)%smc(3)
     mean_smc4     = mean_smc1 + background(t,e)%smc(4)
    enddo
    mean_smc1     = mean_smc1/Ne
    mean_smc2     = mean_smc2/Ne
    mean_smc3     = mean_smc3/Ne
    mean_smc4     = mean_smc4/Ne

    mean_wood     = sum(background(t,:)%wood)       /Ne
    mean_lfmass   = sum(background(t,:)%lfmass)     /Ne
    mean_stmass   = sum(background(t,:)%stmass)     /Ne
    mean_rtmass   = sum(background(t,:)%rtmass)     /Ne

    mean_qe       = sum(output(t,:)%qe)        /Ne
    mean_qh       = sum(output(t,:)%qh)        /Ne
    mean_nee      = sum(output(t,:)%nee)       /Ne
    write(fid,'(                                &
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6)')               & 
     mean_sfctmp,mean_sfcspd,mean_sfcprs,mean_q2,mean_lwrad,          &
     mean_swrad,mean_prcprate,                                        &
     mean_smc1,mean_smc2,mean_smc3,mean_smc4,                         &
     mean_rtmass,mean_wood,mean_lfmass,mean_stmass,                   &
     mean_qe,mean_qh,mean_nee
   enddo
   close(fid)
  endif

   e = 1
   fname = 'output.out'
   open(fid,file=trim(fname),status='replace')

   do t = 1,Nt
    write(fid,'( i7,i5,f7.3,                                          &
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6)')               & 
     date(t,:),time(t), &
     forcing(t,e)%sfctmp, forcing(t,e)%sfcspd, forcing(t,e)%sfcprs,   &
     forcing(t,e)%q2, forcing(t,e)%lwrad, forcing(t,e)%swrad,         &
     forcing(t,e)%prcprate, state(t,e)%smc(1:4),                      &
     state(t,e)%rtmass, state(t,e)%wood,                              &
     state(t,e)%lfmass, state(t,e)%stmass,                            &
     output(t,e)%qe, output(t,e)%qh, output(t,e)%nee
   enddo
   close(fid)

! ensemble output files
! do e = 1,Ne
!  
!  if (perturb) then
!   if (data_assim) then
!    fname = 'enks_'
!   else
!    fname = 'open_'
!   endif
!   if (e.lt.10) then
!    write(es1,'(i1)') e
!    fname = trim(fname)//es1
!   elseif (e.lt.100) then
!    write(es2,'(i2)') e
!    fname = trim(fname)//es2
!   elseif (e.lt.1000) then
!    write(es3,'(i3)') e
!    fname = trim(fname)//es3
!   endif 
!   fname = trim(fname)//'.out'
!  else
!   fname = 'output.out'
!  endif
!  open(fid,file=trim(fname),status='replace')
!
!  do t = 1,Nt
!   write(fid,'(i4,2x,i2,2x,i2,2x,i2,                                 &
!                f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
!                f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
!                f17.6,f17.6,f17.6,f17.6,f17.6,f17.6)')               & 
!    date(t,:),                                                       &
!    forcing(t,e)%sfctmp, forcing(t,e)%sfcspd, forcing(t,e)%sfcprs,   &
!    forcing(t,e)%q2, forcing(t,e)%lwrad, forcing(t,e)%swrad,         &
!    forcing(t,e)%prcprate, state(t,e)%smc(1:4),                      &
!    state(t,e)%rtmass, state(t,e)%wood,                              &
!    state(t,e)%lfmass, state(t,e)%stmass,                            &
!    output(t,e)%qe, output(t,e)%qh, output(t,e)%nee
!  enddo
!  close(fid)
!
!  if (data_assim) then
!   fname = 'back_'
!   if (e.lt.10) then
!    write(es1,'(i1)') e
!    fname = trim(fname)//es1
!   elseif (e.lt.100) then
!    write(es2,'(i2)') e
!    fname = trim(fname)//es2
!   elseif (e.lt.1000) then
!    write(es3,'(i3)') e
!    fname = trim(fname)//es3
!   endif 
!   fname = trim(fname)//'.out'
!   open(fid,file=trim(fname),status='replace')
!
!   do t = 1,Nt
!   write(fid,'(i4,2x,i2,2x,i2,2x,i2,                                 &
!                f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
!                f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
!                f17.6,f17.6,f17.6,f17.6,f17.6,f17.6)')               & 
!     date(t,:),                                                      &
!     forcing(t,e)%sfctmp, forcing(t,e)%sfcspd, forcing(t,e)%sfcprs,  &
!     forcing(t,e)%q2, forcing(t,e)%lwrad, forcing(t,e)%swrad,        &
!     forcing(t,e)%prcprate, background(t,e)%smc(1:4),                &
!     background(t,e)%rtmass, background(t,e)%wood,                   &
!     background(t,e)%lfmass, background(t,e)%stmass,                 &
!     output(t,e)%qe, output(t,e)%qh, output(t,e)%nee
!   enddo
!   close(fid)
!  endif
! enddo ! open ensemble output files

 endif ! if write model output

 if (calc_obj_fun) then
  call rmse_obj_fun(date,time,state(:,1),output(:,1),Nt)
 endif

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
 character(1) :: es1
 character(2) :: es2
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
  !read(fid,*) SMCDRY
  read(fid,*) SMCMAX
  read(fid,*) SMCREF
  read(fid,*) SMCWLT
 close(fid)

 ! turn conductivity into real-valued space
 DKSAT = 10**DKSAT

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
 
 ! expand soil parameters as fractional intervals
 smcmax = smcdry + smcmax
 smcref = smcdry + (smcmax-smcdry)*smcref
 smcwlt = smcdry + (smcref-smcdry)*smcwlt


 open(fid,file='tbot.txt',action='read')
  read(fid,*) tbot ! 67
 close(fid)

 open(fid,file='time_parms.txt',action='read')
  read(fid,*) LAIM   
  read(fid,*) SAIM    
!  read(fid,*) EPS
 close(fid)

 ! some basic manipulations
 kdt = refkdt * dksat / refdk
 frzfact = (smcmax / smcref) * (0.412 / 0.468)
 frzx = frzk * frzfact

 ! error check on rooting layers
 if (nroot.gt.nsoil) nroot = nsoil

end subroutine redprm

subroutine sim_init(setup,state,perturb,data_assim,Ntimes,Nens)
 use type_decs

 integer, parameter :: fid = 14
 type(state_data)   :: state
 type(setup_data)   :: setup
 logical, intent(out) :: perturb,data_assim
 integer, intent(out) :: Nens
 integer, intent(out) :: Ntimes
 integer :: da_flag
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

 open(fid,file='da_flag.txt')
  read(fid,*) da_flag
 close(fid)
 if (da_flag.gt.0) then
  perturb = .true.
  data_assim = .true.
  nens = da_flag
 elseif (da_flag.lt.0) then
  perturb = .true.
  data_assim = .false.
  nens = -da_flag
 elseif (da_flag.eq.0) then
  perturb = .false.
  data_assim = .false.
  nens = 1
 else
  stop 9813
 endif

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

subroutine perturb_state(setup,state)
 use type_decs
 use noahmp_veg_parameters
 use noahmp_globals
 implicit none

 type(state_data), intent(inout) :: state
 type(setup_data), intent(in)    :: setup
 real, parameter :: sig_sm = 0.005
! real, parameter :: sig_veg = 0.01
 integer, parameter :: N = 1
 real :: eta
 real :: random_normal
 integer :: d

 ! soil moisture
 do d = 1,setup%nsoil
  eta = random_normal()
  state%smc(d) = state%smc(d)+eta*sig_sm
  state%sh2o(d) = state%sh2o(d)+eta*sig_sm
 enddo

! ! plant stores
! eta = random_normal()
! state%lfmass = state%lfmass+eta*(sig_veg*(state%lfmass-50/SLA))
! if (state%lfmass.le.50/SLA) state%lfmass = 50/SLA+0.01
! if (state%lfmass.ge.5000/SLA) state%lfmass = 5000/SLA
!
! eta = random_normal()
! state%stmass = state%stmass+eta*(sig_veg*(state%stmass-0.05/0.003))
! if (state%stmass.le.0.05/0.003) state%stmass = 0.05/0.003+0.01 
!
! eta = random_normal()
! state%rtmass = state%rtmass+eta*(sig_veg*state%rtmass)
! if (state%rtmass.lt.5) state%rtmass = 5.01
!
! state%lai = max(state%lfmass*SLA/1000,0.05)
! state%sai = max(state%stmass*3*0.001,0.05)

 return
end subroutine perturb_state


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

subroutine write_state_data(state)
  use type_decs
  implicit none

 type(state_data), intent(in)  :: state

 integer, parameter :: fid = 14
 integer, parameter :: nsnow = 3
 integer, parameter :: nsoil = 4
 integer :: i 
 
 open(fid,file='init_state', status='replace', action='write')
    write(fid,'(                                 &
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6,f17.6      & 
                 f17.6,f17.6,f17.6,f17.6,f17.6,f17.6)')               & 
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
                 state%smcwtd,  &
                 state%deeprech,  &
                 state%rech  
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

subroutine read_state_data(state)
  use type_decs
  implicit none

 type(state_data), intent(inout)  :: state

 integer, parameter :: fid = 14
 integer, parameter :: nsnow = 3
 integer, parameter :: nsoil = 4
 integer :: i 
 
 open(fid,file='init_state')
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
                 state%smcwtd,  &
                 state%deeprech,  &
                 state%rech  
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
end subroutine
