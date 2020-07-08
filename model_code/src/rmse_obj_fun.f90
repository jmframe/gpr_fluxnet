subroutine rmse_obj_fun(mdate,mtime,state,output,Nt)!ofval,errcode)

 use type_decs
 implicit none

 ! ------------------------------------------------------------------
 ! variable declarations
 ! dimensions
 integer, intent(in) :: Nt
 integer, parameter :: Dx = 4
 integer :: ti, xi
 integer, dimension(Dx) :: Gx

 ! dates
 integer, dimension(Nt,2) :: mdate, odate
 real, dimension(Nt) :: mtime, otime
 real :: dummy

 ! obs/mod vars
 real, dimension(Nt,Dx) :: modl, obsv

 ! model vars
 type(state_data),   intent(in), dimension(Nt) :: state
 type(output_data),  intent(in), dimension(Nt) :: output

 ! objective function variables
 real, dimension(Dx)    :: sse, nsmean, nsmeandiff

 ! I/O parms
 integer, parameter :: fid = 136

 ! ------------------------------------------------------------------
 ! read observation file
 open(fid,file='obs.txt')
  do ti = 1,Nt
   read(fid,*) odate(ti,:),otime(ti),obsv(ti,1:3),dummy,obsv(ti,4)
  enddo ! times
 close(fid)

 ! ------------------------------------------------------------------
 ! check dates
 do ti = 1,Nt
  if ((odate(ti,1).ne.mdate(ti,1)).or.  &
      (odate(ti,2).ne.mdate(ti,2)).or.  &
      (otime(ti)  .ne.mtime(ti))) then
    print*, 'mismatch_obs_dates'
    stop 111
   return
  endif
 enddo ! times 

 ! ------------------------------------------------------------------
 ! extract pertinent modeled dimensions
 do ti = 1,Nt
  modl(ti,1) = output(ti)%qe 
  modl(ti,2) = output(ti)%qh
  modl(ti,3) = state(ti)%smc(1)
!  modl(ti,4) = state(ti)%smc(2)
  modl(ti,4) = output(ti)%nee
 enddo ! times 

 ! ------------------------------------------------------------------
 ! calcualte squared errors for each output dimension
 sse = 0.
 Gx = 0
 nsmean = 0. 
do ti = 1,Nt
  do xi = 1,Dx
   if ((obsv(ti,xi).gt.-9990).and.(modl(ti,xi).gt.-9990)) then
    sse(xi) = sse(xi) + ((obsv(ti,xi) - modl(ti,xi))**2)
    Gx(xi) = Gx(xi) + 1
    ! Calculate the record mean for a Nash Sutcliffe efficiency
    nsmean(xi) = nsmean(xi) + obsv(ti,xi)
    ! calcualte anomaly (diff from mean) for each output dimension
    ! for the Nash-Sutcliffe efficiency
   endif 
  enddo 
 enddo

 ! ------------------------------------------------------------------
 ! calcualte anomaly (diff from mean) for each output dimension
 ! for the Nash-Sutcliffe efficiency
 do xi = 1,Dx
  nsmean(xi) = nsmean(xi)/Gx(xi)
 enddo
 nsmeandiff = 0.
 do ti = 1,Nt
  do xi = 1,Dx
   if ((obsv(ti,xi).gt.-9990).and.(modl(ti,xi).gt.-9990)) then
    nsmeandiff(xi) = nsmeandiff(xi) + ((obsv(ti,xi) - nsmean(xi))**2)
   endif 
  enddo 
 enddo

 ! ------------------------------------------------------------------
 ! write to output file - Mean Sum of Squared Error
 !open(fid,file='noahmp_objfun.out')
 !do xi = 1,Dx
 ! write(fid,*) sse(xi)/Gx(xi),','
 !enddo
 !close(fid)

 ! ------------------------------------------------------------------
 ! write to output file - Root Mean Sum of Squared Error
 open(fid,file='noahmp_objfun.out')
 do xi = 1,Dx
  write(fid,*) sqrt(sse(xi)/Gx(xi)),','
  if (xi.eq.3) then
   print*, 'RMSE = ', sqrt(sse(xi)/Gx(xi))
  endif
 enddo
 close(fid)

 ! ------------------------------------------------------------------
 ! write to output file - AS NASH-SUTCLIFFE EFFICIENCY
 ! NOTE: can't do 1-(sse/nsmeandiff) because ostrich only minimizes the obj.
 ! So I just removed the 1-
 !open(fid,file='noahmp_objfun.out')
 !do xi = 1,Dx
 ! write(fid,*) (sse(xi)/nsmeandiff(xi)),','
 !enddo
 !close(fid)

 ! ------------------------------------------------------------------
 return
end subroutine
