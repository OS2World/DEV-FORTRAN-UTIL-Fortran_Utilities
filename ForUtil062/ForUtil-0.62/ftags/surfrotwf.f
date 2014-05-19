#line 1 "surf2aar.fpp"
* vi: set sw=1 ts=8 :
#line 21
*****************************************************************************
* 
#line 29
* file: $Id: surf2aar.fpp,v 1.32 1996/05/31 01:12:21 sad Exp sad $
* date: $, $Date: 1996/05/31 01:12:21 $  (delta  , YY-MM-DD, HH:MM:SS)
* 
* Purpose       : compute resonances in front of a jellium surface bu means
*                 of complex rotation;
*                 here the wave functions get rotated to allow for non-analytic
*                 potentials
* 
* Implementation: Xiazhou Yang, Stefan A. Deutscher  ($Author: sad $)
* Language      : ANSI f77
*
* Version control using RCS:
* run rcs -c'* ' surf2aar.f,v
* to get a * with a space as a comment sign, rather than the default c 
*                
* History       :
*
*  $Log: surf2aar.fpp,v $
*  Revision 1.32  1996/05/31 01:12:21  sad
*  Changed v2c to v2d for potential data, nothing big.
*
* Revision 1.31  1996/05/31  00:37:13  sad
* Added routine to allow file name to be given on VAX and SGI.
*
* Revision 1.30  1996/05/30  21:23:46  sad
* Implemented Associated Legendre Polynomials for both Sun and DEC Alpha.
* Changed command line to also read m, use internal read, and work on
* both Sun and DEC alpha.
*
*  Revision 1.29  1996/05/29 02:52:01  sad
*  Put in code from SLATEC lib (netlib) to generate normalized associated
*  Legendre functions
*  inlcude gauleg.f and above code
*  removed implicit types in main code and some subroutines
*  can handle m >= 0 from now on.
*
* Revision 1.28  1996/05/20  17:19:14  sad
* changed some things for vax file names. Works. Next version will
* clean up implicit type statements.
*
* Revision 1.27  1996/01/04  21:29:05  sad
* Rewrote GetDataFileName to accept everything and make a string.
*
* Revision 1.26  1996/01/04  20:46:49  sad
* added some more file names for ReadV05. Will be refined.
*
* Revision 1.25  1996/01/02  18:45:48  sad
* Changed output header a bit to give the right potential.
* Set xcFlag to true to include dVxc.
*
* Revision 1.24  1996/01/02  00:36:18  sad
* just added some RCS stuff to the output section.
*
*  Revision 1.23  1996/01/01 23:03:08  sad
*  implemented V05, seems to work. Try using inlcudes, though!
*
*  Revision 1.22  1995/12/20 16:36:53  sad
*  Put in new job cards for IBM (up to 1 GB of memory, new password,
*  up to 1440 min of CPU) and changed tabs to spaces for IBM.
*
* Revision 1.21  1995/12/06  20:37:09  sad
* some *** EDIT HERE *** added.
*
* Revision 1.20  1995/12/06  19:34:25  sad
* added stuff for no_loop conditional compile directive. works
*
* Revision 1.19  1995/04/13  21:42:23  sad
* Production code. After this major revisions start (Taking the long source
* file apart into smaller units which will compile faster and allow for easy
* inclusion in the test_pot routine which also shall give us the top of
* the barrier at a certain position at some point).
*
* Revision 1.18  1995/03/21  17:24:18  sad
* So. Now everything is nice. The version where I screwed up in
* November 1994 was 1.11. So there.
*
*  3/95  implementing potential VsurfMe04 that is a image potential like
*        VmeSurf01 which is cut off at the maximum of VsurfMe02, and can
*        be shifted like VsurfMe03
* 11/94  implemented potential VsurfMe03 that allows to shift the ionic
*        jellium edge around, hence allowing for the test of the dependence
*        of the 1s state curve on the actual ionic potential (Specular
*        Reflexion Model vs. RPA)
*  9/94  moved dimension ND, LD to make file (for convergence tests with
*        different basis sizes); same should be done with number of
*        integration nodes sometime; and back to corrected potential with
*        xc effects
*  7/94  command line parameters for UNIX version, maybe easier to do with
*        internal reads but works
*  6/94  added potential VsurfMe02 that uses different ionic image
*        potential, derived from dielectric response formalism (with
*        dispersion) and then fitted to some sort of ionic image plane
*  4/94  ported to be OS indepndent (cms, UNIX, ..), since NAG lib got
*        available on UNIX1 machine; S.D.
* 11/93  version for stabilization method derived, ported to be OS
*        independent (CMS, UNIX, OS/2); see surf3as*.zip, surf4as*.zip,
*        surf5as*.zip; S.D.
*  6/93  more realistic potential used, S.D.
*  5/93  rewritten for rotation of wave functions, X.Y.
* 12/92  first version by X.Y.; rotation of cut-off potential
*
*
* fortran files : surf2aar.f, atof.f, atoi.f, chtoi.f, isdigit.f, getparam.f
* pascal files  :
* C files       :
* Header files  :
*
* libraries     : nag lib; so far available on utkux1, utkap1, cms/mvs
*
* on UNIX
* =================================
* compile with  : -c -native -O3
* link with     : -o surfrot -lnag
* makefile      : makefile.surf2aar
*
* on utkap1 (OpenVMS, AlphaMachine)
* =================================
* compile with  : fortan filename.f /float=g_float
* link with     : link filename,naglib /lib
* run with      : run filename.exe
*
* References    :
* Comments      : lots of things need to be done:
*               + remove all those darn {\em implicit} statements
*               + change to Gauss-Laguerre-Integration;
*               + reduce number of integration nodes
*               + change output to be more useful (no of EV, header, ..)
*               + move to fweb
*               + make the UNIX/CMS/VAX ifdefs apply to comments, too.
*               + link/include with vsurflib.f and the rest of it on UNIX,
*                 or include the files otherwise somehow for CMS ..
*               + add cpp directive for os2, update paramstr etc.
* 
*****************************************************************************
#line 165
* #define 30 30
* #define 15 15

C file on UNIX and cms originally called surf2a.fbtempc,
C now called surf2aar.f  : surf2a-version; ar-wavefunction-rotation
C file on pc called surf2a.f01
C Begin of the FORTRAN code to compute resonances of an atom on a jellium
C surface, using a potential POTENTIAL
C Code written by Xiazhou Yang 1993 and extended by Stefan Deutscher
#line 176
C The program uses about 33 MB of static memory (16 bit double precision)
C with nd=30 and ld=15 and mw=1000
C Locate the parts that may need to be edited (parameters, potentials,
C output routines and change them, if needed. The are marked by a
C comment like "C *** EDIT HERE ***".

C In order to speed up the compilation while testing it is a nice idea
C to use the compiler fortvs2. This requires the first lines with the
C JCL commands commented out, until and including the @PROCESS line.
C The matrix dimensions (nd, ld) should be set to smaller values.
C The line holding the open statement should be un-commented and the
C output device in all the write statements should be changed from
C 6 (console / file so106 in batch job) to the one specified in the
C open statement (11?). Specifying 6 here did not work, alas!
C Now, the file can be compiled and after that be run with the run
C command.
C If not already done in the PROFILE EXEC file the necessary libraries
C should be invoked first with: getlibs nag imsl fortvs2 <ENTER>.
#line 196
C The next three lines are input lines. The maximum value $n_{max}$ of
C the radial quantum number $n_r$ taken into account is denoted with ND,
C LD stands for the maximum value $l_{max}$ and MW is the number of nodes
C used for the {\sc Gauss-Legendre}-Integration.
C Thus, ND represents the size of the base.
C *** EDIT HERE ***

      program surf2aar 
      implicit none
*
*     Parameter variables
*
      integer          nd
      parameter       (nd = 30)      ! *** EDIT HERE ***
      integer          ld, ld1
      parameter       (ld = 15)      ! *** EDIT HERE ***
      parameter       (ld1 = ld + 1)
      integer          mw
      parameter       (mw = 1000)
      integer          nd1
      parameter       (nd1 = nd+1)
      integer          md
      parameter       (md = nd1*(ld+1))
#line 221
      double precision zero, fourth, half, one, two, four
      parameter       (zero   = 0.00d0)
      parameter       (fourth = 0.25d0)
      parameter       (half   = 0.50d0)
      parameter       (one    = 1.00d0)
      parameter       (two    = 2.00d0)
      parameter       (four   = 4.00d0)

*
*     Local variables
*
      double precision alfi(md), alfr(md), beta(md), c2, cc, cf, cfl, dl
      double precision dn, dtheta, eps, f(mw), f1s, f2s, r, r0, r1, rend
      double precision logsn0
      double precision s2, sn0, ss, theta, theta_min
      double precision vi, vr, w(mw), x(mw), x1, xb
      double precision xr
      double precision Plm(0 : ld)          ! hold P_l^m(x) temporarily

      double precision theta_max

      double precision zIonJell            ! position of ionic jellium edge
*
      integer i, i1, iai, iar, ib, ibi, ibr, ifail, ii, iter(md), ivi
      integer ivr, l, lb, m, n, nb, nt, ntheta
      integer l_low,  l_upp

      logical matv
*
*     Common variables
*
      double precision ai(md,md), ar(md,md), bi(md,md), br(md,md)
      double precision si(mw,0:nd,0:ld), sr(mw,0:nd,0:ld)
      double precision vb(mw,0:ld,0:ld)
      common /bigcom/sr,si,ar,ai,br,bi,vb
#line 258
      integer          mR, mC
      parameter        (mR = 100)        ! must be the same as in data file
      parameter        (mC = 100)        ! must be the same as in data file
      double precision xRV(1:mR), xCV(1:mC)
      double precision VionM(1:mR, 1:mC)
      double precision dVxcM(1:mR, 1:mC)
      logical          xcFlag            ! do or do not include dVxc part
      common /V05com/  xRV, xCV, VionM, dVxcM, xcFlag

      character*(72)   filename          ! or how long ever ...
#line 270
* ----------------------------------------------------------------------------
* external functions
* ----------------------------------------------------------------------------
      double precision potential     ! wrapper function around potentials

* ----------------------------------------------------------------------------
* external procedures
* ----------------------------------------------------------------------------
      external NormAssLegFunc        ! actually, this gets included.

      external ReadPotTable          ! read potential data
#line 283
* ----------------------------------------------------------------------------
* first executable statement:
* ----------------------------------------------------------------------------
C Now, we get constants or input parameters

C Here, we have two lines of input:
C THETA is the angle \theta of the complex co-ordinate rotation, and R is
C the distance from the atom to the surface. THETA and R are the
C parameters to play around with\dots
C REnd replaces infinity in the integration routine 0..\infty,
C it is a cut-off parameter which should be checked.
C sn0 is the last input parameter to play with, it denotes the n_0,
C i.~e. 1/Sturmian_Parameter for the sturmians
C m is the magnetic quantum number (which may need to be implemented
C     in the loop that builds the sturmians)

C *** EDIT HERE ***   Constants ***
#line 302
*  ---------------      set up default values --------------------------- 
      r         = 10.0d00           ! ion-surface
      theta_min = 0.05d00

      theta_max = 0.45d00
      dtheta    = 0.01d00
      ntheta    = nint((theta_max - theta_min) / dtheta)

      REnd      = 450.0d00
      sn0       = 2.0d0
      m         = 0
*     zIonJell  = 0.0d0          ! ionic jellium edge defined so far
                                 ! in function potential()
      zIonJell  = 0.7d0          ! ionic jellium edge defined so far
                                 ! in function potential()
#line 319
* #if defined (1) || (vax) || (os2)

*  ---------------------------------------------------------------------
*  in UNIX, VMS, or OS/2 ask for cmd line parameters, unless no_loop 
*  is specified, which makes the whole code act as benchmark code, so far.
*  It may be neccessary to allow to specify r, sn0, theta, for
*  those jobs where one value for theta hits the cpu time limits already.
*  ---------------------------------------------------------------------

      call GetParameters(m, r, sn0,
     &                   theta_min, theta_max, dtheta, ntheta)
*     call GetParameters(m, r, sn0,
*    &                   theta, theta_max, dtheta, ntheta)
#line 334
      theta = theta_min
      call Initialize()    ! initialize stuff for the code
#line 338
      xcFlag = .true.          ! do or do not include dVxc contribution
      xcFlag = .false.         ! do or do not include dVxc contribution

      filename ='Dontknowyet'
      call ReadV05ionData(filename, r, mR, mC, xRV, xCV, VionM, dVxcM)

      call WriteHeader(nd, ld, m, mw, theta_min, theta_max, r,
     &                 REnd, sn0, zIonJell, xcFlag)
#line 348
C The subroutine GAULEG computes the weights W_i and abcissas X_i
C of the nodes C used for the Gauss-Legendre-Integration. The routine is
C taken from "Numerical
C Recipes" by Press, Teukolsky, Vetterling and Flannery.

      call gauleg(-one,one,x,w,mw)
#line 356
C The following lines of code generate the associated Legendre functions,
C SR(i,0,l) = P^m_l(x_i), by calling a routine from the NetLib (Slatec).

      l_low = 0
      l_upp =ld

      do i=1,mw
       xb=x(i)
       call NormAssLegFunc(Plm, l_low, l_upp, m, xb)
*      call nPlm0(Plm, l_low, l_upp, m, xb)      ! testing, for m=0

       do l=0,ld
        sr(i,0,l)= Plm(l)
       enddo
      enddo

* R1 replaces infinity in the integration routine, it is a cut-off
* parameter which should be checked , especially for higher n, since
* <r>_n = 3/2 n^2.

      r1=REnd
      r0=r1*half

C The next lines until to the label 200 compute the effective potential
C V_eff
C xr is actually r_i
C XB=\cos(\alpha) where \alpha is the angle between r and z.

      do i=1,mw
       xr=(x(i)+one)*r0

       do ii=1,mw
        XB=X(II)
        f(ii)=potential(xr,xb,r)
       enddo

C Here we start the integration (which is replaced by the sum accoding
C to the Gauss-Legendre scheme) to compute the effective potential
C V^{eff}_{l'l}(r_i). This part ends at label 200.
       do l=0,ld

        do lb=l,ld
         dn=zero
         do ii=1,mw
          dn=dn+f(ii)*sr(ii,0,l)*sr(ii,0,lb)*w(ii)
         enddo
        vb(i,l,lb)=dn
        vb(i,lb,l)=dn
        enddo

       enddo

      enddo

      logsn0 = dlog(sn0)              ! needed in the loop.

C Now all initialization stuff is done.
#line 416
      do nt=1,ntheta              ! Start big loop over \theta:

       theta = theta+dtheta
#line 421
C Compute stuff for the roatation
       cc=dcos(theta)
       ss=dsin(theta)
       c2=dcos(two*theta)
       s2=dsin(two*theta)
C From here on until to label 270 the programme generates the rotated
C Sturmian functions S(r_i,n,l)=SR + i*SI.
       do l=0,ld
        dl=dfloat(l)
        cfl=logsn0

        do n=1,l+1
         dn=dfloat(n)
         cfl=cfl-dlog(dn*(dn+dl))
        enddo

        cfl=half*cfl

        do i=1,mw
         xb=(x(i)+one)*r0/sn0
         cf=(dl+one)*dlog(two*xb)-xb*cc+cfl
         f1s=xb*ss-(dl+one)*theta
         sr(i,0,l)=dcos(f1s)
         si(i,0,l)=dsin(f1s)
         f1s=dsqrt(two/(dl+two))
         vr=dl+one-xb*cc
         vi=xb*ss
         sr(i,1,l)=f1s*(vr*sr(i,0,l)-vi*si(i,0,l))
         si(i,1,l)=f1s*(vr*si(i,0,l)+vi*sr(i,0,l))

         do n=2,nd
          dn=dfloat(n)
          f1s=one/dsqrt((one+one/(dn+dl))*(one+(two*dl+one)/dn))
          f2s=dsqrt((one-one/(dn+dl))*(one-one/dn)*(one+two*dl/dn))
          vr=two*(one+(dl-xb*cc)/dn)
          vi=two*xb*ss/dn
          sr(i,n,l)=f1s*(vr*sr(i,n-1,l)-vi*si(i,n-1,l)-f2s*sr(i,n-2,l))
          si(i,n,l)=f1s*(vr*si(i,n-1,l)+vi*sr(i,n-1,l)-f2s*si(i,n-2,l))
         enddo

         do 25 n=0,nd
          xb=dabs(sr(i,n,l))
          if(xb.eq.zero) then
           goto 22
          else
           sr(i,n,l)=sr(i,n,l)/xb*dexp(dlog(xb)+cf)
          endif
22        xb=dabs(si(i,n,l))
          if(xb.eq.zero) then
           goto 25
          else
           si(i,n,l)=si(i,n,l)/xb*dexp(dlog(xb)+cf)
          endif
25       continue

         enddo    ! do i=1,mw

        enddo    ! do l=0,ld

C here starts te computation of A_{nl,n'l'} and B_{nl,n'l,}.
C it ends at label 110.
         do 110 ii=1,md
         l=ii/nd1
         n=l*nd1
         if(n.eq.ii)l=l-1
         n=ii-l*nd1-1
C here the computation of V_{nl,nl} starts. it ends at label 40.
         f1s=zero
         f2s=zero

         do i=1,mw
          vr=r0*w(i)*(sr(i,n,l)*sr(i,n,l)-si(i,n,l)*si(i,n,l))
          vi=r0*w(i)*(si(i,n,l)*sr(i,n,l)+sr(i,n,l)*si(i,n,l))
          f1s=f1s+vr*vb(i,l,l)
          f2s=f2s+vi*vb(i,l,l)
         enddo

C The following four lines compute the real and imaginary parts of
C A_{nl,nl} and B_{nl,nl}, ar, ai, and br, bi, respectively
         ar(ii,ii)=f1s*cc+f2s*ss+c2*half
         ai(ii,ii)=f2s*cc-f1s*ss-s2*half
         br(ii,ii)=sn0*sn0
         bi(ii,ii)=zero
C Now V_{nl,n'l'} is to be computed. The task is ended at label 110.
         if(ii.eq.md) goto 110
           do 120 ib=ii+1,md
           lb=ib/nd1
           nb=lb*nd1
           if(nb.eq.ib)lb=lb-1
           nb=ib-lb*nd1-1
           f1s=zero
           f2s=zero

           do i=1,mw
            vr=r0*w(i)*(sr(i,n,l)*sr(i,nb,lb)-si(i,n,l)*si(i,nb,lb))
            vi=r0*w(i)*(si(i,n,l)*sr(i,nb,lb)+sr(i,n,l)*si(i,nb,lb))
            f1s=f1s+vr*vb(i,l,lb)
            f2s=f2s+vi*vb(i,l,lb)
           enddo

C The computation of A and B for different pairs of indices
C (nl \neq n'l') starts here and is also finished  at label 120
           ar(ii,ib)=f1s*cc+f2s*ss
           ai(ii,ib)=f2s*cc-f1s*ss
           br(ii,ib)=zero
           bi(ii,ib)=zero
           if(lb.eq.l) then
             if(nb.eq.n-1) then
              x1=dsqrt(dfloat(n*(2*l+1+n))/dfloat((n+l+1)*(n+l)))
              ar(ii,ib)=ar(ii,ib)+c2*x1*fourth
              ai(ii,ib)=ai(ii,ib)-s2*x1*fourth
              br(ii,ib)=-sn0*sn0*half*x1
             endif
             if(nb.eq.n+1) then
              x1=dsqrt(dfloat((n+1)*(2*l+2+n))/dfloat((n+l+1)*(n+l+2)))
              ar(ii,ib)=ar(ii,ib)+c2*x1*fourth
              ai(ii,ib)=ai(ii,ib)-s2*x1*fourth
              br(ii,ib)=-sn0*sn0*half*x1
             endif
           endif
           ar(ib,ii)=ar(ii,ib)
           ai(ib,ii)=ai(ii,ib)
           br(ib,ii)=br(ii,ib)
           bi(ib,ii)=bi(ii,ib)
120        continue
110      continue

C Now we get some other handsome parameters required by subroutine f02gjf
         ifail=0
         eps=1.d-20
         matv=.false.
         iar=md
         iai=md
         ibr=md
         ibi=md
         ivr=md
         ivi=md
C The subroutine f02gjf is part of the NAG library and
C (block-)diagonalizes the matrix, i.e. solves the complex
C generalized eigenvalue problem
         call f02gjf(md,ar,iar,ai,iai,br,ibr,bi,ibi,eps,alfr,alfi,
     &   beta,matv,vr,ivr,vi,ivi,iter,ifail)

         do i=1,md
*         ar(i,1)=alfr(i)/beta(i)    ! that's how it was
*         ai(i,1)=alfi(i)/beta(i)    ! that's how it was
          alfr(i)=alfr(i)/beta(i)
          alfi(i)=alfi(i)/beta(i)
         enddo

C AR now holds the real part of the energy of the ith resonance, E_r(i),
C and AI holds the imaginary part of the energy of this resonance,
C E_i(i). E(i) = E_r(i) +i* E_i(i)

C Here the E(i)'s get sorted according to their real part's value
C The subroutine MOVER is just made to move the values around.
C Sorting is ended at label 150
         do 150 i=1,md-1
         do 160 i1=1,i

*        if(ar(i+1,1).ge.ar(i1,1)) goto 160  ! that's how it was
*        call mover(i+1,i1,ar(1,1))           ! that's how it was
*        call mover(i+1,i1,ai(1,1))           ! that's how it was

         if(alfr(i+1).ge.alfr(i1)) goto 160
         call mover(i+1,i1,alfr(1))
         call mover(i+1,i1,alfi(1))
         goto 150
160      continue
150      continue

C Finally, the output part!

      call WriteData(md, theta, alfr, alfi)
      call WriteFooter()
#line 600
      enddo     ! do nt=1,ntheta  (big loop over theta)
#line 607
	 close(6)
         stop
         end
#line 612
* ========================================================================
* And here we have a bunch of subroutines:
* ========================================================================
#line 617
* ========================================================================
* Initialize all kinds of things
* ========================================================================
      subroutine Initialize()
      implicit none
      external InitSlatec
#line 641
*     --------------------------------------------------------------------
*     Initialize extended precision arithmetic for Slatec library:
*     --------------------------------------------------------------------
      call InitSlatec()

*     --------------------------------------------------------------------
*     Open output files, in case redirection of screen output doesn't
*     work. (It does for UNIX, VAX, IBM, OS/2 though). This is handled
*     nicer for the VAX in GetParameters, but may be an issue still on
*     the IBM main frame. Uncomment if needed.
*     --------------------------------------------------------------------
C     open(6,file='/file output a')    ! valid cms file name
C     open(6,file='file.output')       ! valid 1,vms,os2 file name

      return
      end
#line 659
* ------------------------------------------------------------------------
* Initialize extended precision arithmetic for Slatec library:
* ------------------------------------------------------------------------
      subroutine InitSlatec()
      implicit none
      integer          irad, nradpl, nbits, ierror
      double precision dzero
      external dxset           ! from Slatec / Netlib
*     ----------------------------------------------------------------
*     this works on UNIX, OS/2, and on the DEC, but setting all to zero
*     and using the appropriate settings in i1mach.f/fpp seems more
*     reliable.
*     ----------------------------------------------------------------
*     irad   = 2               ! guessed
*     nradpl = 10              ! guessed
*     dzero  = 1.0d+32         ! guessed
*     nbits  = 32              ! guessed

      irad   = 0               ! guessed
      nradpl = 0               ! guessed
      dzero  = 0.0d0           ! guessed
      nbits  = 0               ! guessed

      ierror = 0
      call dxset (irad, nradpl, dzero, nbits, ierror)

      if (ierror .ne. 0) then
       write(6,*) '# dxset called in InitSlatec: ierror =', ierror
      endif

      return
      end                   ! of InitSlatec()

* ------------------------------------------------------------------------
* The subroutine GetParameters fetches command line arguments in
* UNIX, VMS, and OS/2.
* If '-h', '-H', '-?' are given or too few command arguments are
* given it calls the Syntax explanation, if no arguments are given at
* all it exits w/o changing the input, just calculates ntheta.
* In case specified, it also opens an output file (good for vax)
* ------------------------------------------------------------------------
      subroutine GetParameters(m, r, sn0, theta, 
     &                         theta_max, dtheta, ntheta)
      implicit none
*     --------------------------------------------------------------------
*     procedure head
*     --------------------------------------------------------------------
      double precision r, sn0, theta, theta_max, dtheta
      integer          m, ntheta
*     --------------------------------------------------------------------
*     internal declarations
*     --------------------------------------------------------------------
      character*255 pname, argstr, ofile
      integer       narg, needed
      parameter    (needed = 6)
*     --------------------------------------------------------------------
*     external procedures
*     --------------------------------------------------------------------
      external paramstr
*     --------------------------------------------------------------------
*     first executable
*     --------------------------------------------------------------------
      pname  = ' '        ! initialize with blanks
      argstr = ' '        ! initialize with blanks
      call paramstr(pname, argstr, narg)

      if (narg .eq. 0) then
        ntheta = nint((theta_max - theta) / dtheta)
	return
      elseif ((narg .eq. -1) .or. (narg .lt. needed)) then
	call Syntax(pname)
      elseif (narg .eq. needed) then            ! only arguments given
	read(argstr, *) m, r, sn0, theta, theta_max, dtheta
        ntheta = nint((theta_max - theta) / dtheta)
      elseif (narg .eq. needed+1) then          ! output file given, too
        ofile  = ' '                            ! initialize with blanks

*     --------------------------------------------------------------------
*     Internal read of file name works with Sun and OS/2 but not with
*     Vax or SGI, so:
*     --------------------------------------------------------------------

	read(argstr, *) m, r, sn0, theta, theta_max, dtheta
	call GetLastWord(argstr, ofile)
#line 747
        ntheta = nint((theta_max - theta) / dtheta)

	open(6,file=ofile,status='unknown')     ! reopen stdout
      endif

      end   ! of subroutine GetParameters
#line 755
* ------------------------------------------------------------------------
* The subroutine GetLastWord() gets the filename also from SGI and DEC
* where the internal reads don't work as on the Sun or on OS/2
* ------------------------------------------------------------------------
#line 1 "getlastword.f"
*****************************************************************************
* vi:set wm=0:
*
* file: $Id:$
* date: $Date:$  (YY-MM-DD, HH:MM:SS)
*
* Purpose       : Get the last word of a command line string,
*                 since internal reads work on the Sun but not
*                 on the SGI or DEC Alpha machine
*                 read(string, *) x, y, filename
*
* Implementation: Stefan A. Deutscher  ($Author: sad $)
* Language      : ANSI f77
*
* fortran files :
*
* References    : 
* Comments      : 
*
*  $Log:$
*
*****************************************************************************

      subroutine GetLastWord(argstr, ofile)
      implicit none
      character*(*) argstr, ofile
      integer i, end_lastword, begin_lastword, len_lastword

      end_lastword      = 0
      begin_lastword    = 0

      i = len(argstr)          ! go to the very end
      do while ((i .gt. 0) .and. (argstr(i:i) .eq. ' '))
        i = i -1	       ! step back while it is blank
      enddo
      end_lastword = i

      do while ((i .gt. 0) .and. (argstr(i:i) .ne. ' '))
        i = i -1	
      enddo
      begin_lastword    = i+1   ! step forward to non-blank again

      len_lastword = end_lastword - begin_lastword + 1

      ofile(1:len_lastword) = argstr(begin_lastword:end_lastword)
      return
      end

* ------------------------------------------------------------------------
* end.
* ------------------------------------------------------------------------
#line 762 "surf2aar.fpp"
* ------------------------------------------------------------------------
* The subroutine Syntax() prints the syntax explanation and exits
* ------------------------------------------------------------------------
      subroutine Syntax(pname)
      implicit none
      character*(*) pname
      character*79  s

      s = 'Parameters:  [m d  sn0  theta  theta_max  dtheta [outfile]]'
      write(6,*)'Syntax: ',pname
      write(6,*)
      write(6,*) s
      write(6,*)
      write(6,*)'-help, -HELP, -\? (can be abbreviated) for this help'
      write(6,*)'m         : magnetic quantum number, m=0 ...',15
      write(6,*)'d         : ion to jellium-edge distance [a.u.]'
      write(6,*)'sn0       : Sturmian parameter to use'
      write(6,*)'            sn0 = 2 for n = 2 e.g., optimize'
      write(6,*)'theta     : starting angle for rotation'
      write(6,*)'theta_max : maximum angle for complex rotation'
      write(6,*)'dtheta    : step width for rotation'
      write(6,*)'outfile   : output file, esp. useful on the vax'
      write(6,*)'            if not given, output goes to the screen'
      write(6,*)'            output redirection still works on unix'
      stop
      end  ! subroutine Syntax
#line 790
* ------------------------------------------------------------------------
* Include routine to get parameter string for 1/os2
* ------------------------------------------------------------------------
#line 1 "unixargs.f"
*****************************************************************************
* vi:set wm=0:
*
* file: $Id: unixargs.f,v 1.2 1996/05/28 18:32:19 sad Exp $
* date: $Date: 1996/05/28 18:32:19 $  (YY-MM-DD, HH:MM:SS)
*
* Purpose       : 
*
* Implementation: Stefan A. Deutscher  ($Author: sad $)
* Language      : ANSI f77
*
* fortran files :
*
* References    : 
* Comments      : 
*
*  $Log: unixargs.f,v $
c Revision 1.2  1996/05/28  18:32:19  sad
c removed comment line, changed -? option
c
*  Revision 1.1  1996/05/28 17:18:59  sad
*  Initial revision
*
*
*****************************************************************************

*---------------------------------------------------------------------------
*
* the follwing subroutine returns programme name, parameter string
* (also called argument string or command line), and number of 
* parameters read.
*
* pname     -- on entry anything
*              on exit  programme name as invoked
* argstr -- on entry anything, will be preserved unless cmd line
*              was specified
*              on exit: original string or command line as give (except
*              programme name)
* narg      -- integer, on entry: anything
*           -- on exit: 
*              number of parameters returned in argstr 
*              0   -- only command name returned
*             -1   -- command line flag was -h, -H, -?, -\?, 
*                     give Syntax help
*
* This works on 1 and OS/2 (g77)
* No testing is done to overflow of the command line; the Fortran Standard
* will cause this to just be cut off.
*
* Invoke, e.g. with
*     character*255 argstr, pname
*     integer       narg
*     external      paramstr
*     double precision x1, x2, x3, x4 ,x5
*     ....
*     call paramstr(pname,argstr,narg)
*     if (narg.lt.0) call Syntax()
*     write(6,*) pname, argstr
*     read(argstr,*) x1, x2, x3
*     write(6,*) x1, x2, x3
*      ...
*---------------------------------------------------------------------------
      subroutine paramstr(pname, argstr, narg)
      implicit none
* --------------------------------------------------------------------------
* subroutine head
* --------------------------------------------------------------------------
      character*(*) pname, argstr
      integer       narg, arglen, i, j, k
      character*255 argv

* --------------------------------------------------------------------------
* external functions
* --------------------------------------------------------------------------
      integer  iargc 
      external iargc

* --------------------------------------------------------------------------
* external subroutines
* --------------------------------------------------------------------------
      external getarg

* --------------------------------------------------------------------------
* first executable statement
* --------------------------------------------------------------------------

      narg = iargc()            ! get numer of cmd line arguments
      call getarg(0, pname)     ! get program name
      if (narg.eq.0) return     ! return if that's all there was

      call getarg(1, argv)      ! get syntax or atom-surface distance

      if ((argv(1:2).eq.'-h') .or.      ! anybody asked for help ?
     &    (argv(1:2).eq.'-H') .or.
     &    (argv(1:2).eq.'-?')) then
       narg   = -1              ! return flag set to call for Syntax help
       argstr = argv
       return
      endif

      k = 1                       ! current position in argstr
      do j=1, narg                ! loop over all arguments
       call getarg(j, argv)       ! get argument no. j
       arglen = len(argv)         ! and its length

* ------------------------------------------------------------------------
*      copy argument over until blank is found, // pads with blanks
*      and can't be used here.
* ------------------------------------------------------------------------
       i = 1                      ! reset current position in argv
       do while ((i.le.arglen).and.(.not.(argv(i:i).eq.' ')))
        argstr(k:k) = argv(i:i)
        i = i + 1                 ! update position in argv
        k = k + 1                 ! update position in argstr
       enddo

       argstr(k:k) = ' '          ! put a blank inbetween
       k = k + 1                  ! update position in argstr
      enddo                       ! done with it.

      return
      end
#line 803 "surf2aar.fpp"
* ------------------------------------------------------------------------
* The subroutine GAULEG computes the weights W_i and abcissas X_i of
* the nodes used for the Gauss-Legendre-Integration. The routine is
* taken from "Numerical Recipes" by Press, Teukolsky, Vetterling and
* Flannery.
* ------------------------------------------------------------------------
#line 1 "dgauleg.f"
*****************************************************************************
* 
* file: $Id:$
* date: $Date:$  (delta  , YY-MM-DD, HH:MM:SS)
* 
* Purpose       : get nodes for Gauss Legendre integration
* 
* Implementation: Stefan A. Deutscher  ($Author: sad $)
* Language      : ANSI f77
*                
*  $Log:$
*
* fortran files : 
* pascal files  :
* C files       :
* Header files  :
*
* References    : Numerical Recipes in Fortran
* Comments      : some compilers may want dcos() and dabs() explicitly
* 
*****************************************************************************
#line 24
C The subroutine GAULEG computes the weights W_i and abcissas X_i of
C the nodes used for the Gauss-Legendre-Integration. The routine is
C taken from "Numerical Recipes" by Press, Teukolsky, Vetterling and
C Flannery.
C
      subroutine gauleg(x1,x2,x,w,n)
      implicit none
      integer n
      double precision x1,x2,x(n),w(n)
      double precision eps
      parameter (eps=3.d-14)
      integer i,j,m
      double precision p1,p2,p3,pp,xl,xm,z,z1
      m=(n+1)/2
      xm=0.5d0*(x2+x1)
      xl=0.5d0*(x2-x1)
      do 12 i=1,m
        z=cos(3.141592654d0*(i-.25d0)/(n+.5d0))
1       continue
          p1=1.d0
          p2=0.d0
          do 11 j=1,n
            p3=p2
            p2=p1
            p1=((2.d0*j-1.d0)*z*p2-(j-1.d0)*p3)/j
11        continue
          pp=n*(z*p1-p2)/(z*z-1.d0)
          z1=z
          z=z1-p1/pp
        if(abs(z-z1).gt.eps)goto 1
        x(i)=xm-xl*z
        x(n+1-i)=xm+xl*z
        w(i)=2.d0*xl/((1.d0-z*z)*pp*pp)
        w(n+1-i)=w(i)
12    continue
      return
      end

*************************************************************************** 
*  end of file dgauleg.f
*************************************************************************** 
#line 811 "surf2aar.fpp"
* #if !defined (1)
* ------------------------------------------------------------------------
* Inlcude subroutine which returns the normalized associated Legendre
* functions for arbitrary m, from NetLib (SLATEC package)
* P_{l=0\to ld}^m(xb).
* ------------------------------------------------------------------------
#line 1 "NormAssocLegFunc.fpp"
*****************************************************************************
* vi:set wm=0:
*
* file: $Id:$
* date: $Date:$  (YY-MM-DD, HH:MM:SS)
*
* Purpose       :  Return the normalized associated Legendre functions
*                  using the SLATEC library routine dxlegf.f
*
* Implementation: Stefan A. Deutscher  ($Author: sad $)
* Language      : ANSI f77
*
* fortran files : lots,  all from SLATEC (NetLib).
*   NormAssocLegFunc.f dxadd.f dxadj.f dxlegf.f dxpmu.f dxpmup.f dxpnrm.f 
*   dxpqnu.f dxpsi.f dxqmu.f dxqnu.f dxred.f dxset.f fdump.f i1mach.f 
*   j4save.f xercnt.f xerhlt.f xermsg.f xerprn.f xersve.f xgetua.f 
* test routine  : at least for m=0  nalfTest.f
*
* References    : ftp.netlib.com, SLATEC
* Comments      :
*    + needs dxset (also from SLATEC) to be run before calling
*    + 15 must be set to the same as in the main code, either by
*      inlcuding this file in the main code, or by hand, or by using
*      features of the pre-processor, of fweb, or an include file for 
*      15 everywhere.
*
*  $Log:$
*
*****************************************************************************

      subroutine NormAssLegFunc(Plm, l_low, l_upp, m, x)
      implicit none
* -------------------------------------------------------------------------
* function head
* -------------------------------------------------------------------------
      integer          ld, ld1
      parameter       (ld  = 15)
*     parameter       (ld  = 100)          ! for testing only
      parameter       (ld1 = ld + 1)
      double precision Plm(0 : ld)
      integer          l_low, l_upp        ! could be real as well.
      integer          m
      double precision x
* -------------------------------------------------------------------------
* internal parameters
* -------------------------------------------------------------------------
      double precision dzero
      parameter       (dzero = 0.0d0)
* -------------------------------------------------------------------------
* internal declarations
* -------------------------------------------------------------------------
      double precision theta, dl_low
      double precision vlm(1 : ld1)
      integer          ilm(1 : ld1)
      integer          l_diff, ierror, l
      integer          id
      parameter       (id = 4)             ! see docs of dxlegf.f
      logical          neg_x, odd_l
* -------------------------------------------------------------------------
* external declarations
* -------------------------------------------------------------------------
      external dxlegf                     ! from slatec/netlib, calc. P_n^m

* -------------------------------------------------------------------------
* procedure block starts
* -------------------------------------------------------------------------

* dxlegf() expects theta to be in half-open interval (0,pi/2], i.w.
* x to be in [0, 1).
* From A/S: 8.2.3 one can see that for nu, mu integer the following
* still holds (like for m=0) for all m=mu:
* P_l^m(-x) = (-)^l P_l^m(x).
*

      if (x .lt. dzero) then            ! x is negative
       x = -x                           ! flip sign
       neg_x = .true.                   ! true if x<0
      else
       neg_x = .false.                  ! x not negative
      endif

      theta  = acos(x)
      l_diff = l_upp - l_low
      dl_low = dble(l_low)
      ierror = 0

      call dxlegf(dl_low, l_diff, m, m, theta, id, vlm, ilm, ierror)

      if (ierror .ne. 0) then
       write(6,*) '# Stopped in NormAssLegFunc: ierror = ', ierror
*      Maybe check which ilm <> 0, that's where under/overflow is
       stop               ! that may be too drastic
      endif

      do l = 0, ld       ! map vlm(1:ld+1) array back to Plm(0:ld)
       Plm(l) = vlm(l+1)  
      enddo
* ------------------------------------------------------------------------
* flip sign for negative args and odd l only
* ------------------------------------------------------------------------
      if (neg_x .eqv. .true.) then
       do l = 0, ld
        odd_l = .not.(mod(l,2).eq.0)            ! true if odd(l)
        if (odd_l .eqv. .true.) Plm(l) = -Plm(l)
       enddo
      endif

      return
      end
* -------------------------------------------------------------------------
* end of subroutine NormAssLegFunc().
* -------------------------------------------------------------------------
#line 818 "surf2aar.fpp"
* #endif

* ------------------------------------------------------------------------
* Subroutine nPlm0 which returns the normalized associated Legendre
* functions (i.e. Legendre polynomials) for m=0, from A/S, Xiazhou's
* implementation. P_{l=0\to ld}^{m=0}(xb).
* It agrees to better than 1.0e-13 compared to the Slactec
* at least with the parameters guessed for the call to dxset().
* ------------------------------------------------------------------------

      subroutine nPlm0(Xlm, l_low, l_upp, m, xb)
      implicit none
      integer          ld
      parameter       (ld  = 15)
      double precision one, half, four
      parameter       (one = 1.0d0, half = 0.5d0, four = 4.0d0)
      double precision Xlm(0:ld)
      double precision xb
      integer          l_low, l_upp, m

      integer          l
      double precision dl

      if ((m .ne. 0) .or. (l_low .ne. 0)) then
       write(6,*) '# nPlm0: l_low=',l_low,' m=',m,' Both must be 0.'
       stop
      endif

      Xlm(0)=0.70710678118654752440d0
      Xlm(1)=1.22474487139158904909d0*xb

      do l=2,l_upp
       dl=one/dfloat(l)
       Xlm(l)=dsqrt(four-dl*dl)*xb*Xlm(l-1)
     &   -(one-dl)*dsqrt((one+half*dl)/(one-1.5d0*dl))*Xlm(l-2)
      enddo

      return
      end
#line 859
* ------------------------------------------------------------------------
* The subroutine MOVER, used above to sort the energy values of the
* resonances
* ------------------------------------------------------------------------
      subroutine mover(n,i,x)
      implicit none
      integer          i, n
      double precision x(n)
      integer          j, m
      double precision z

      z=x(n)
      do 10 j=1,n-i
       m=n-j
       x(m+1)=x(m)
10    continue
      x(i)=z
      return
      end
#line 880
C The routine WriteOutput must be adapted to the used potential in
C order to provide sufficent information for the evaluation of  the
C results. Output all parameters used etc., one in each line,
C preceeded with a # sign (comment for GnuPlot)
C Note: The bigcom common block cannot be used in a subroutine on
C the IBM, so there must be a way to put things into either the
C main programme or a subroutine, depending on the target architecture.

      subroutine WriteHeader(nd, ld, m, mw, theta_min, theta_max, r,
     &                       REnd, sn0, zIonJell, xcFlag )
      implicit none
      integer          nd, ld, m, mw
      double precision theta_min, theta_max, r, REnd, sn0, zIonJell
      logical          xcFlag

1     format(1x,a,e12.5)
2     format(1x,a,i12)
3     format(1x,a,1p1e10.4)

      write(6,*)'#  file:'
      write(6,*)'#  date:'
      write(6,*)'#  rotation of wave functions'
      write(6,*)'# source code:'
      write(6,*)'#   file     : $RCSfile: surf2aar.fpp,v $'
      write(6,*)'#   revision : $Revision: 1.32 $'
      write(6,*)'#   date     : $Date: 1996/05/31 01:12:21 $'
      write(6,*)'#   author   : $Author: sad $'
      write(6,*)'#   state    : $State: Exp $'
      write(6,*)'#'
      write(6,*)'#  n_r  max     = ', nd
      write(6,*)'#  l    max     = ', ld
      write(6,*)'#  m            = ', m
      write(6,*)'#  Nodes Leg.   = ', mw
      write(6,3)'#  theta_min    = ', theta_min
      write(6,3)'#  theta_max    = ', theta_max
      write(6,*)'#  d (ion-surf) = ', r
      write(6,*)'#  RCut         = ', REnd
      write(6,*)'#  n_0  (St.)   = ', sn0
      write(6,*)'#  z0e  (el.)   = 0.70'              ! in commons!
      write(6,3)'#  z0i  (ion)   = ', zIonJell        ! in commons!

*  Put in line for electronic potential, and version information!
*  Ionic image potential
*  Ionic core potential  :
*  write out lambda, u0, etc 
*  write out Ionic image plane :
*  write out Electronic image plane :
*  put all this in common blocks rather then manually!
*  also xcFlag.
#line 939
      if (xcFlag .eqv. .true.) then
       write(6,*)'#  Potential type: JJW+AE+LDF+dVxc     potential'
      else
       write(6,*)'#  Potential type: JJW+AE+LDF w/o dVxc potential'
      endif

      write(6,*)'#  Z_eff        =    1.00'
      write(6,*)'#  V_0          =    0.5746'   ! imag, cut, and corr
C     write(6,*)'#  V_0          =    0.2420'   ! self
      write(6,*)'#  begin of data'

      return
      end           ! of subroutine WriteHeader

      subroutine WriteData(md, theta, Er, Ei)
      implicit none
      integer  md
      double precision theta
      double precision Er(md), Ei(md)

      integer  i

      write(6,2) '# Starting theta = ', theta
      do i=1,md
cc     if(dabs(Er(i)).le.0.7d0) then
*       write(6,1) theta, Er(i), Ei(i)   ! change in the end !
        write(6,1) Er(i), Ei(i)
cc     endif
      enddo
* 1     format(1x,1p1e10.3,1p2e22.10)               ! change in the end !
1     format(1x, 3e22.10)
2     format(1x, a, 1p1e10.3)
      return
      end          ! of subroutine WriteData

      subroutine WriteFooter()
      implicit none

      write(6,*)'# end of data'
      return
      end           ! of subroutine WriteFooter
#line 983
C The part with the potential:  choose a potential here
C Note that -16.4 eV correspond to -0.60294117647058823529 a.u.
C *** EDIT HERE ***

      function potential(xr, xb, r)
*     ------------------------------------------------------------------------
*     declaration of function type and parameters
*     ------------------------------------------------------------------------
*     implicit double precision (a-z)
      implicit none
      double precision potential
      double precision xr, xb, r

*     ------------------------------------------------------------------------
*     internal declarations and common blocks
*     ------------------------------------------------------------------------

      double precision z0, zIonJell, lambda, u0
#line 1003
      integer          mR, mC
      parameter        (mR = 100)        ! must be the same as in data file
      parameter        (mC = 100)        ! must be the same as in data file
      double precision xRV(1:mR), xCV(1:mC)
      double precision VionM(1:mR, 1:mC)
      double precision dVxcM(1:mR, 1:mC)
      logical          xcFlag            ! do or do not include dVxc part
      common /V05com/  xRV, xCV, VionM, dVxcM, xcFlag
#line 1013
*     ------------------------------------------------------------------------
*     external functions
*     ------------------------------------------------------------------------
      double precision VsurfMe01, VsurfMe02, VsurfMe03, VsurfMe04
      external         VsurfMe01, VsurfMe02, VsurfMe03, VsurfMe04

      double precision VsurfMe05
      external         VsurfMe05
#line 1024
*     ------------------------------------------------------------------------
*     function program block starts
*     ------------------------------------------------------------------------

        z0        = 0.70d0        ! electronic image plane
        zIonJell  = 0.70d0        ! ionic jellium edge (for now zIonJell = z0)
        lambda    = 1.25d0        ! for VsurfMe01/02 with saturation at -0.574
        u0        = 1.1492d0      ! for VsurfMe01/02 with saturation at -0.574
*       lambda    = 0.60d0        ! for saturation at value of electr. self-im.
*       u0        = 0.4840d0      ! for saturation at value of electr. self-im.
#line 1043
        potential = VsurfMe05(xr, xb, r, z0, lambda, u0, zIonJell,
     &                        xRV, mR, xCV, mC, VionM, dVxcM, xcFlag)

        return
      end
#line 1052
* ========================================================================
* function returns floor(x: real):integer
* ========================================================================
       function floor(x)
       implicit none
       integer          floor
       double precision x

       double precision zero
       parameter       (zero = 0.0d0)
       integer          lower

         lower = int(x)
         if ((x.lt.zero).and.(real(lower).ne.x)) lower = lower - 1 
         floor = lower
         return
       end

* ========================================================================
* function returns ceil(x: real):integer
* ========================================================================
       function ceil(x)
       implicit none
       integer          ceil
       double precision x

       double precision zero
       parameter       (zero = 0.0d0)
       integer          upper

         upper = int(x)
         if ((x.gt.zero).and.(real(upper).ne.x)) upper = upper + 1
         ceil = upper
         return
       end

* ========================================================================
* subroutine returns FloorCeil(x: real; floor, ceil : integer)
* Note that ilower = iupper if x is a whole number
* This should be a bit faster than two separate calls since int()
* is invoked only once.
* ========================================================================
       subroutine FloorCeil(x, ilower, iupper)
       implicit none
       integer          ilower, iupper
       double precision x

       double precision zero
       parameter       (zero = 0.0d0)

         ilower = int(x)
         if (real(ilower).eq.x) then            ! x is whole number
           iupper = ilower
         else
           if (x.lt.zero) ilower = ilower - 1 
           iupper = ilower + 1
         endif
         return
       end
#line 1113
      subroutine dhunt(xx,n,x,jlo)
      integer jlo,n
      double precision x,xx(n)
      integer inc,jhi,jm
      logical ascnd
      ascnd=xx(n).gt.xx(1)
      if(jlo.le.0.or.jlo.gt.n)then
        jlo=0
        jhi=n+1
        goto 3
      endif
      inc=1
      if(x.ge.xx(jlo).eqv.ascnd)then
1       jhi=jlo+inc
        if(jhi.gt.n)then
          jhi=n+1
        else if(x.ge.xx(jhi).eqv.ascnd)then
          jlo=jhi
          inc=inc+inc
          goto 1
        endif
      else
        jhi=jlo
2       jlo=jhi-inc
        if(jlo.lt.1)then
          jlo=0
        else if(x.lt.xx(jlo).eqv.ascnd)then
          jhi=jlo
          inc=inc+inc
          goto 2
        endif
      endif
3     if(jhi-jlo.eq.1)return
      jm=(jhi+jlo)/2
      if(x.gt.xx(jm).eqv.ascnd)then
        jlo=jm
      else
        jhi=jm
      endif
      goto 3
      end
#line 1157
************************************************************************
*     vi:set sm ruler ai:
*
*   file: vMe05ion.f (vMe05ion.f,v 1.5 1996/01/01 16:39:45 sad Exp)
*   date: Date: 1996/01/01 16:39:45 
*   purpose: perform bilinear interpolation to provide the surface
*         potential from a smaller table.
*
************************************************************************

************************************************************************
*
*
* The data in the table has to be arranged in the following way:
* Comment lines can be in there at any time, starting with a hashmark
*  xR[1]   xC[1]  dummy dummy Vion[1,1]   dVxc[1,1]
*    .       .      .     .      .           .
*  xR[1]   xC[k]  dummy dummy Vion[1,k]   dVxc[1,k]
*    .       .      .     .      .           .
*  xR[1]   xC[mC] dummy dummy Vion[1,mC]  dVxc[1,mC]
* <empty line>
*  xR[2]   xC[1]  dummy dummy Vion[2,1]   dVxc[2,1]
*    .       .      .     .      .           .
*  xR[2]   xC[k]  dummy dummy Vion[2,k]   dVxc[2,k]
*    .       .      .     .      .           .
*  xR[2]   xC[mC] dummy dummy Vion[2,mC]  dVxc[2,mC]
* <empty line>
*    .       .      .     .      .           .
* <empty line>
*  xR[mR]  xC[1]  dummy dummy Vion[mR,1]  dVxc[mR,1]
*    .       .      .     .      .           .
*  xR[mR]  xC[k]  dummy dummy Vion[mR,k]  dVxc[mR,k]
*    .       .      .     .      .           .
*  xR[mR]  xC[mC] dummy dummy Vion[mR,mC] dVxc[mR,mC]
*
*
* usage:
*
*     integer          mR, mC
*     parameter        (mR = 10)        ! must be the same as in data file
*     parameter        (mC = 10)        ! must be the same as in data file
*     double precision REnd
*     parameter        (REnd = 450.0)   ! must be the same as in data file
*     double precision xRV(1:mR)
*     double precision xCV(1:mC)
*     double precision VionM(1:mR, 1:mC)
*     double precision dVxcM(1:mR, 1:mC)
*     double precision xR, xC, Vion, dVxc
*      
*   [ ... ]
*
*     call ReadPotTable("filename", xRV, xCV, mR, mC, VionM, dVxcM)
*      
*   [ ... ]
*
*     call PotSRMLDF(xR, xC, xRV, mR, eqdR, xCV, mC, eqdC,
*    &               VionM, dVxcM, Vion, dVxc, iRlo, iClo)
*
*   [ ... ]
*
************************************************************************

************************************************************************
* Read Potential Table from file and store in 2D matrix for interpolation
************************************************************************
      subroutine ReadPotTable(filename, mR, mC, xRV, xCV, VionM, dVxcM)
      implicit none
      character*(*)    filename              ! arbitrary length 
      integer          mR, mC
      double precision xRV(1:mR)
      double precision xCV(1:mC)
      double precision VionM(1:mR, 1:mC)
      double precision dVxcM(1:mR, 1:mC)

*     ------------------------------------------------------------------------
*     declarations and common blocks
*     ------------------------------------------------------------------------
      integer          fileunit, k, l, is
      double precision dummy, xRt

*     ------------------------------------------------------------------------
*     procedure body starts
*     ------------------------------------------------------------------------

      fileunit = 11
      open(unit=fileunit, file=filename,status='old', iostat = is)

      if (is.ne.0) then
        write(6,*) 'Error in subroutine: ReadPotTable.'
        write(6,*) 'File ', filename, ' not found.'
        write(6,*) 'The programme has been aborted.'
        write(6,*) 'Compile and run impot2.c for that distance.'
	write(6,*)
        stop
      endif

      do k = 1, mR
        do l = 1, mC
 8001     continue               ! skip comments until data begins
          read(fileunit,*,err=8001,end=8002)
     &    xRt, xCV(l), dummy, dummy, VionM(k,l), dVxcM(k,l)

*         write(6,*) xRt, xCt, VionM(k,l), dVxcM(k,l)    ! for debugging only
        enddo
        xRV(k) = xRt             ! get R value
        read(fileunit,*)         ! skip empty line between different R values
      enddo

 8002 continue
      close(fileunit)
      return
      end
#line 1272
* ***************************************************************************
* Description:
* Returns the indices corresponding to the elements xV[ilo], xV[ihi] of an
* equidistantly spaced vector xV[1..mV], surrounding the value x. 
* In case a grid point is hit, x = xV[i], it returns ilo = ihi
* As a bonus, delta is returned.
*
* xVlo = xV[1] <= xV[ilo] <= x <= xV[ihi] <= xV[mV] = xVhi
*
* Input:
*         x, xVlo, xVhi, mV, 
* Ouptut:
*         delta, ilo, ihi
* ***************************************************************************
      subroutine GetEqdVecIndex(x, xVlo, xVhi, mV, delta, ilo, ihi)
      implicit none
      double precision x, xVlo, xVhi, delta
      integer          mV, ilo, ihi

* ----------------------------------------------------------------------------
* external subroutines
* ----------------------------------------------------------------------------
      external FloorCeil

*     this error checking can be disabled for speed:

*     if (.not.((xVlo.le.x).and.(x.le.xVhi))) then
*       pause 'GetEqdVecIndex: x is out of range'
*     endif

      delta = (xVhi - xVlo) / dfloat(mV - 1)
      call FloorCeil((x - xVlo) / delta + 1.0d0, ilo, ihi)
      return
      end

* ***************************************************************************
* Description:
* Returns the indices corresponding to the elements xV[ilo], xV[ihi] of an
* arbitrarily spaced vector xV[1..mV], surrounding the value x. 
* In case a grid point is hit, x = xV[i], it returns ilo = ihi.
* To make the hunt efficient, the index of the last found value (ilo) has
* to be provided as input; the routine hunts from there and returns the
* new value. At the first hunt, set ilo to 1.
*
* xVlo = xV[1] <= xV[ilo] <= x <= xV[ihi] <= xV[mV] = xVhi
*
* Input:
*         x, xV, mV, ilo
* Ouptut:
*         ilo, ihi
* ***************************************************************************
      subroutine HuntVecIndex(x, xV, mV, ilo, ihi)
      implicit none
      double precision x 
      integer          mV, ilo, ihi
      double precision xV(1:mV)

* ----------------------------------------------------------------------------
* external subroutines
* ----------------------------------------------------------------------------
      external  dhunt
#line 1335
*     this error checking can be disabled for speed:

*     if (.not.((xV(1).le.x).and.(x.le.xV(mV)))) then
*       write(6,*) 'HuntVecIndex: x is out of range!'
*       write(6,*) 'xV[1] = ', xV(1), ' x = ', x, ' xV[mV] = ', xV(mV)
*       pause 'HuntVecIndex: x is out of range!'
*     endif

      call dhunt(xV, mV, x, ilo)                   ! from numerical recipes

      if (x.ne.xV(ilo)) then
        ihi = ilo+1     ! not on a grid point, hence not on the upper end either
      else
        ihi = ilo       ! get upper limit
      endif

      return
      end

* ***************************************************************************
* Description:
* Get the weighting factors t, v for linear interpolation
* 
* Input:
*  x         - dble, value to interpolate for
*  xV[1..mV] - dble. vector with all the values available for interpolation
*  ilo, ihi  - int, indices of surrounding vector elements, equal in case
*              x = xV[i]
*
* Output:
* t          - dble., weight for xV[ilo],  
* v          - dble., weight for xV[ihi],  v = 1 - t
*
* ***************************************************************************

      subroutine GetLIntWeights(x, xV, mV, ilo, ihi, t, v)
      implicit none
      double precision x
      integer          mV 
      double precision xV(1:mV)
      integer          ilo, ihi
      double precision t, v

* ----------------------------------------------------------------------------
*     declarations and common blocks
* ----------------------------------------------------------------------------
      double precision one, zero, xlo, xhi
      parameter       (one  = 1.0d0)
      parameter       (zero = 0.0d0)

* ----------------------------------------------------------------------------
*     procedure body starts
* ----------------------------------------------------------------------------

*     this error checking can be disabled for speed:

*     if ((ilo.lt.1).or.(mV.lt.ihi)) then
*       write(6,*) '# GetLIntWeights: index out of range!'
*       write(6,*) '#                 ', 1, ilo, ihi, mV
*       pause
*     endif
#line 1398
      if (ilo.eq.ihi) then                   ! right on a grid point
        t = zero
        v = one
      else                                   ! we must interpolate
        xlo = xV(ilo)
        xhi = xV(ihi)
        t = (x - xlo) / (xhi - xlo)
        v = one - t
      endif

      return
      end

* ***************************************************************************
* Perform 2D interpolation on table holding ionic potential from SRM + LDF.
* Parameters:
*    xR    - dbl,  value for R (0 .. REnd)
*    xC    - dbl,  value for cos(theta)  (-1 .. 1)
*    xRV[1..mR] - vector with nodes in R direction
*    mR    - int, nodes in table in R direction (about 20 .. 100)
*    eqdR  - logical, equidistant in R direction
*    xCV[1..mC] - vector with nodes in cos(theta) direction
*    mC    - int, nodes in table in cos(theta) direction (about 20 .. 100)
*    eqdC  - logical, equidistant in cos(theta) direction
*    VionM - dbl, matrix holding Vion_{SRM+LDF}(R[k],cos(theta)[l])
*    dVxcM - dbl, matrix holding xc correction
*    Vion  - dbl, interpolated potential at xR, xC
*    dVxc  - dbl, interpolated xc contribution at xR, xC
*    iRlo  - int, last R index found, set to 1 in very first call
*    iClo  - int, last C index found, set to 1 in very first call
*
* external files: flooceil.f
* 
* ***************************************************************************
      subroutine PotSRMLDF(xR, xC, xRV, mR, eqdR, xCV, mC, eqdC,
     &                     VionM, dVxcM, Vion, dVxc, iRlo, iClo)
      implicit none
      double precision xR, xC
      integer          mR, mC
      double precision xRV(1:mR)
      double precision xCV(1:mC)
      logical          eqdR, eqdC
      double precision VionM(1:mR, 1:mC), dVxcM(1:mR, 1:mC)
      double precision Vion, dVxc

*     ------------------------------------------------------------------------
*     declarations and common blocks
*     ------------------------------------------------------------------------
      integer          iRhi, iRlo, iChi, iClo
      double precision xR1, xR2, xC1, xC2, dummy
      double precision t, u, v, w
#line 1451
* ----------------------------------------------------------------------------
* external subroutines
* ----------------------------------------------------------------------------
      external GetEqdVecIndex, HuntVecIndex, GetLIntWeights
#line 1457
* ----------------------------------------------------------------------------
*     procedure body starts
* ----------------------------------------------------------------------------

      xR1 =  xRV(1)     ! should be zero here
      xR2 =  xRV(mR)    ! should be REnd here
      xC1 =  xCV(1)     ! should be -1 here
      xC2 =  xCV(mC)    ! should be  1 here

*     ------------------------------------------------------------------------
*     get indices and values of grid points surrounding (xR, xC) either
*     hunting or directly calculating it, depending on eqd[RC] flags.
*     if we are on a grid point, iRlo=iRhi or iClo=iChi are possible !
*     dummy holds the grid spacing returned, which we don't need
*     ------------------------------------------------------------------------

      if (eqdR.eqv..true.) then
        call GetEqdVecIndex(xR, xR1, xR2, mR, dummy, iRlo, iRhi)
      else
        call HuntVecIndex(xR, xRV, mR, iRlo, iRhi)
      endif

      if (eqdC.eqv..true.) then
        call GetEqdVecIndex(xC, xC1, xC2, mC, dummy, iClo, iChi)
      else
        call HuntVecIndex(xC, xCV, mC, iClo, iChi)
      endif

*     ------------------------------------------------------------------------
*     prepare interpolation in xR and xC directions: get weights t,v,u,w
*     ------------------------------------------------------------------------
      call GetLIntWeights(xR, xRV, mR, iRlo, iRhi, t, v)
      call GetLIntWeights(xC, xCV, mC, iClo, iChi, u, w)

*     ------------------------------------------------------------------------
*     perform the 2D interpolation
*     ------------------------------------------------------------------------

      Vion = v * w * VionM(iRlo, iClo) + t * w * VionM(iRhi, iClo)
     &     + t * u * VionM(iRhi, iChi)     + v * u * VionM(iRlo, iChi)

      dVxc = v * w * dVxcM(iRlo, iClo) + t * w * dVxcM(iRhi, iClo)
     &     + t * u * dVxcM(iRhi, iChi)     + v * u * dVxcM(iRlo, iChi)

      return
      end

************************************************************************
* end of file
************************************************************************

* ***************************************************************************
* procedure GetDataFileName(d, filename)
* ***************************************************************************
      subroutine GetDataFileName(d, filename)
      implicit none
      character*(*)    filename
      double precision d

      character*5       d_string
      character*5       name_head
      character*12      name_tail

      parameter        (name_head = 'v2d.d')
      parameter        (name_tail = '.mw0100.gaus')
#line 1526
      integer           i

*     --------------------------------------------------------------
*     internal write, to convert double d into string d_string:
*     replace blanks with zeros afterwards
*     --------------------------------------------------------------
      write(d_string,'(f5.1)') d    
      i = 1
      do while ((i.le.len(d_string)).and.(d_string(i:i).eq.' '))
         d_string(i:i) = '0'
         i = i + 1
      enddo

*     --------------------------------------------------------------
*     // is used for string concatenation in fortran to put the 
*     filename together and finally allow the data to be found in 
*     a given directory. 
*
*     filename = 'director_name/'//filename
*
*     This is essential for 1 batch jobs on different machines,
*     but must be removed for cms/vax, where the file names anyway 
*     must be cut to something meaningless cryptic. *** EDIT HERE ***
*     --------------------------------------------------------------------

      filename = name_head//d_string//name_tail

      filename = '/scrtch/sad/surf/data/'//filename
#line 1556
      return
      end
#line 1560
* ***************************************************************************
* procedure ReadV05ionData()
* Description:
* Read data for ionic potential from data file corresponding to distance,
* stop when there is none.
* The whole if .. then .. elseif sequence could be simplified with some
* string manipulation and internal reads / writes, but this does the job.
*       filename = 'v2a.d000.0.mw0100.equi'      ! for equidist. data
*       filename = 'v2a.d000.0.mw0100.gaus'      ! for data at gaussian nodes
* ***************************************************************************
      subroutine ReadV05ionData(filename, d, mR,mC,xRV,xCV,VionM,dVxcM)
      implicit none
      character*(*)    filename
      double precision d
      integer          mR, mC
      double precision xRV(1:mR), xCV(1:mC)
      double precision VionM(1:mR, 1:mC)
      double precision dVxcM(1:mR, 1:mC)

      external ReadPotTable, GetDataFileName

      call GetDataFileName(d, filename)

      write(6,*)'# Reading data for V05ion from file:'
      write(6,*)'# ', filename
      call ReadPotTable(filename, mR, mC, xRV, xCV, VionM, dVxcM)

      return
      end
#line 1591
C test potential of screened Coulomb interaction.

      function VScrCoulomb(xr,xb,r)
      implicit double precision (a-h,o-z)
        z=dabs(xr*xb)
        VScrCoulomb = -dexp(-z)/xr
        return
      end
#line 1601
C test potential: Stark potential
C Coulomb potential plus linear term -1/r+a*z,
C where a is small (1.0e-5 ?)
#line 1606
      function VStark(xr,xb,r)
      implicit double precision (a-h,o-z)
        a = 1.0d-05
        z = xr*xb
        VStark = a*z -1.0d00/xr
        return
      end
#line 1615
C test potential: Stark potential (asymmetric)
C Coulomb potential plus linear term -1/r+a*z*\theta(R)
C where a is small (1.0e-5 ?), i.e. the potential is
C a Stark potential till to the position of the atom core and a
C normal core potential for z < 0. This is to check whether one
C can consider the surface potential as some sort of asymmetric
C Stark potential, and wheter or not we see the asymmetric line
C splitting that we saw with the surface potentials
C Note that the ion sits at (0,0,0)

      function VStarkAsymm(xr,xb,r)
      implicit double precision (a-h,o-z)
        a = 1.0d-03
        z = xr*xb
        v = -1.0d00/xr
        if (z.lt.0.0d00)      v = a*z + v
        VStarkAsymm = v
        return
      end
#line 1636
C potential of image interaction, used by Xiazhou originally.
C Here, we have two lines of input, i.~e. adjustable parameters of the
C potential: ZEFF denotes the effective charge z_eff, V0 is V_0.

      function VimXia(xr,xb,r)
      implicit double precision (a-h,o-z)
        zeff = 1.00d0
        v0   = 0.5746d0
        c    = 1.d0/v0+4.d0*r
        ror=r/xr
        if(xb.le.ror) then
          VimXia = -zeff/xr+zeff/dsqrt(4.d0*r*r+xr*xr-4.d0*r*xr*xb)
     1             -1.d0/(c-4.d0*xr*xb)
        else
          VimXia = -v0
        endif
        return
      end

C file Vme_001.f                     09-07-93
C use: write(6,*)'#  Potential type: JJW-AE        potential'
C This is the first line of my surface potential VsurfMe01
C which implements a hopefully more realistic surface potential

C Ion image potential, shifted to surface position at $r = (0,0,d)$
C     (this is for Zeff == 1; may need adjustment)
C      given as:
C      if (z>d) then VzzSh = -1.0/sqrt(z*z + rho*rho)
C               else VzzSh = -1.0/sqrt((2.0*d-z)*(2.0*d-z) +rho*rho)
C reads in the used co-ordinate system where we get
C XR == r, XB == \cos\theta, R == d
#line 1669
        function VzzSh(r,costheta,d)
        implicit double precision (a-h, o-z)
C        double precision r, z, d, costheta
          z = r * costheta
          if (z.ge.d) then
            VzzSh = -1.0d0/r
          else
            VzzSh = -1.0d0/dsqrt(4.0d0*d*(d-z)+r*r)
          endif
        return
        end
#line 1682
C Electronic potential according to Jennings, Jones, Weinert

        function VelSh(z,z0,lambda,u0)
        implicit double precision (a-z)

          a = 2.0d0*U0/lambda-1.0d0
          t = z-z0
          if (z.lt.z0) then
            V     = 0.5d0*(1.0d0-dexp(lambda*t))/t
          else
            V     = - u0/(a*dexp(-u0/a*t)+1.0d0)
          endif
C scale from Rydberg to Hartree
          VelSh = 0.5d0 * V
        return
        end
#line 1700
C total potential as sum of the above
C      here we will use: z0 = 0.7 ; lambda = 1.25; u0 = 1.21

        function Vtot(r,costheta,d,z0,lambda,u0)
        implicit double precision (a-z)
           z    =   r*costheta
           Vtot = - VzzSh(r,costheta,d) + VelSh(z-d,-z0,lambda,u0)
        return
        end
#line 1711
C And, finally, to fit in the code, add -1/r for the ion and

      function VsurfMe01(xr, xb, r, z0, lambda, u0)
      implicit double precision (a-z)
*       z0        = 0.70d0
*       lambda    = 1.25d0
*       u0        = 1.1492d0
        VsurfMe01 = -1.00d0/xr + Vtot(xr, xb, r, z0, lambda, u0)
        return
      end

C This is the last line of my surface potential VsurfMe01

* file Vme_002.f                     06-28-94
* use: write(6,*)'#  Potential type: JJW-AE (corr) potential'
*      for the normal electronic image with exchange etc. and
*      write(6,*)'#  Potential type: JJW-AE (self) potential'
*      for the one with the lower values
* This is the first line of my surface potential VsurfMe02
* which implements a hopefully more realistic surface potential

* Ion image potential, shifted to surface position at $r = (0,0,d)$
*   (this is for Zeff == 1; may need adjustment)
*   given as the same as VsurfMe01 except that we use
*
*                z + a + b*d
*
*   with a = 2.16535 and b = 0.993 instead of z+d
*   a represents some kind of ionic image plane, and both a and b come
*   from a fit to the ``exact'' numerical solution of the formula derived
*   from dielectric response theory and plasmon-pole approximation {\em with}
*   dispersion. This fit holds only for the ion {\em outside} of the solid.
*   For the ion inside we will have a separate potential
*
* reads in the used co-ordinate system where we get
* XR == r, XB == \cos\theta, R == d, see sheet 6-30-94/1 for details
#line 1749
        function VzzSh2(r,costheta,d)
        implicit double precision (a-h, o-z)
C        double precision r, z, d, costheta
        parameter(a = 2.16535d00)
        parameter(b = 0.99305d00)

        z  = r * costheta
        d0 = a + b * d    ! get kind of ionic image plane here
        if (z.ge.d) then
            t      = -d + d0
            VzzSh2 = -1.0d0/dsqrt(t * (t + 2.0d00*z)+r*r)
          else
            t      =  d + d0
            VzzSh2 = -1.0d0/dsqrt(t * (t - 2.0d00*z)+r*r)
          endif
        return
        end

* total potential as sum of the above and the JJW potential
*      here we will use: z0 = 0.7 ; lambda = 1.25; u0 = 1.21

        function Vtot2(r,costheta,d,z0,lambda,u0)
        implicit double precision (a-z)
           z    =   r*costheta
           Vtot2 = - VzzSh2(r,costheta,d) + VelSh(z-d,-z0,lambda,u0)
        return
        end
#line 1778
C And, finally, to fit in the code, add -1/r for the ion and

      function VsurfMe02(xr, xb, r, z0, lambda, u0)
      implicit double precision (a-z)
*       z0        = 0.70d0           
*       lambda    = 1.25d0        ! or  0.60
*       u0        = 1.1492d0      ! and 0.484  for "realistic pot"
        VsurfMe02 = -1.00d0/xr + Vtot2(xr, xb, r, z0, lambda, u0)
        return
      end

C This is the last line of my surface potential VsurfMe02
#line 1792
* file Vme_003.f                     10-28-94
* use: write(6,*)'#  Potential type: JJW-AE (corr) potential'
*      for the normal electronic image with exchange etc. and
*      write(6,*)'#  Potential type: JJW-AE (self) potential'
*      for the one with the lower values and specify zIonJell
* This is the first line of my surface potential VsurfMe03
* It is the same as VsurfMe02, except:
* 
*   Now, however, we allow for an additional parameter that lets us
*   shift around the position of the ionic jellium edge, i.e. the peak
*   in the ionic image potential:
*
* reads in the used co-ordinate system where we get
* XR == r, XB == \cos\theta, R == d, see sheets 6-30-94/1 and
* 11-8-94 for details
#line 1809
        function VzzSh2Im(r,costheta,d, zJell)
        implicit double precision (a-h, o-z)
C        double precision r, z, d, costheta
        parameter(a = 2.16535d00)
        parameter(b = 0.99305d00)

        z  = r * costheta
        zh = z + zJell    ! for compacter code
        d0 = a + b * d    ! get kind of ionic image plane here

        if (zh.ge.d) then
            t  = -d + d0
            s  =  t * (t + 2.0d00*zh)
          else
            t  =  d + d0
            s  =  t * (t - 2.0d00*zh)
          endif
        VzzSh2Im = -1.0d0 / dsqrt(s + r*r + zJell * (zh + z))
        return
        end

* total potential as sum of the above and the JJW potential
*      here we will use: z0 = 0.7 ; lambda = 1.25; u0 = 1.21

        function Vtot3(r,costheta,d,z0,lambda,u0, zJell)
        implicit double precision (a-z)
           z    =   r*costheta
           Vtot3 = - VzzSh2Im(r,costheta,d, zJell) 
     &             + VelSh(z-d,-z0,lambda,u0)
        return
        end
#line 1842
C And, finally, to fit in the code, add -1/r for the ion and

      function VsurfMe03(xr, xb, r, z0, lambda, u0, zJell)
      implicit double precision (a-z)
*       z0        = 0.70d0           
*       lambda    = 1.25d0        ! or  0.60
*       u0        = 1.1492d0      ! and 0.484  for "realistic pot"
        VsurfMe03 = -1.00d0/xr + Vtot3(xr, xb, r, z0, lambda, u0, zJell)
        return
      end

C This is the last line of my surface potential VsurfMe03

* file Vme_004.f                     19-Mar-1995
* use: write(6,*)'#  Potential type: JJW-AE (cut) potential'
*      specify zIonJell
*
* This is the first line of my surface potential VsurfMe04
* It is the same as VsurfMe01, except:
* We cut of the ionic image potential, which would diverge when
* d=z=0, at the maximum value of the Echenique-Abajo potential.
* This also determines the thickness of the ionic potential well.
* The cut-off points can be determined according to sheets 16-Mar-1995
* as |z_c| = -(d -(a+bd)), where a and b are given in Vme02.
* 
* We still allow for an additional parameter that lets us
* shift around the position of the ionic jellium edge, i.e. the peak
* in the ionic image potential in order to fulfill the asymptotic
* conditions for the potential:
*
* reads in the used co-ordinate system where we get
* XR == r, XB == \cos\theta, R == d, see sheets 6-30-94/1 and
* 11-8-94 for details
#line 1877
        function VzzSh4Im(r,costheta,d, zJell)
        implicit double precision (a-h, o-z)
C        double precision r, z, d, costheta
        parameter(a = 2.16535d00)
        parameter(b = 0.99305d00)

        z  = r * costheta
        zj = (a+b*d)      ! this is the ionic image plane location
        zc = zj - d       ! cut-off for image potential
        zh = z + zJell    ! for compacter code

        if (dabs(d - zh).le.zc) then         ! cut off at the value of V_{AE}
*         ct       = dabs(costheta)          ! 
          ct       = costheta
          rt       = d/costheta
          VzzSh4Im = VzzSh2(rt, ct, d)        ! at the surface r = d
        else                                 ! get the image potential:
          s = 0.0d0                                 ! for zh >  d
          if (zh.le.d) s  =  4.0d0 * d * ( d - zh ) ! for zh <= d
          s = s + zJell * (zh + z)    
          VzzSh4Im = -1.0d0 / dsqrt(s + r*r) ! the image potential
        endif
        return
        end

* total potential as sum of the above and the JJW potential
*      here we will use: z0 = 0.7 ; lambda = 1.25; u0 = 1.21

        function Vtot4(r,costheta,d,z0,lambda,u0, zJell)
        implicit double precision (a-z)
           z    =   r*costheta
           Vtot4 = - VzzSh4Im(r,costheta,d, zJell) 
     &             + VelSh(z-d,-z0,lambda,u0)
        return
        end
#line 1914
C And, finally, to fit in the code, add -1/r for the ion and

      function VsurfMe04(xr, xb, r, z0, lambda, u0, zJell)
      implicit double precision (a-z)
*       z0        = 0.70d0           
*       lambda    = 1.25d0        ! or  0.60
*       u0        = 1.1492d0      ! and 0.484  for "realistic pot"
        VsurfMe04 = -1.00d0/xr + Vtot4(xr, xb, r, z0, lambda, u0, zJell)
        return
      end
#line 1927
C This is the last line of my surface potential VsurfMe04

* file Vme_005.f                     27-Dec-1995
* use: write(6,*)'#  Potential type: JJW-AE-LDF-dVxc   potential'
* use: write(6,*)'#  Potential type: JJW-AE-LDF   potential'
*
* This is the first line of my surface potential VsurfMe05
* It is the same as VsurfMe01, except:
* We cut of the ionic image potential, which would diverge when
* d=z=0, at the maximum value of the Echenique-Abajo potential.
* This also determines the thickness of the ionic potential well.
* The cut-off points can be determined according to sheets 16-Mar-1995
* as |z_c| = -(d -(a+bd)), where a and b are given in Vme02.
* 
* We still allow for an additional parameter that lets us
* shift around the position of the ionic jellium edge, i.e. the peak
* in the ionic image potential in order to fulfill the asymptotic
* conditions for the potential:
*
* reads in the used co-ordinate system where we get
* XR == r, XB == \cos\theta, R == d, see sheets 6-30-94/1 and
* 11-8-94 for details

* total potential as sum of the (external) ionic image potential
* PotSRMLDF() and the electronic JJW potential
* here we will use: z0 = 0.7 ; lambda = 1.25; u0 = 1.1492
*
* This will crash unless the Potential data is read first using something
* like
*
*     call ReadPotTable('filename',
*     &                  mR,mC,xRV,xCV,VionM,dVxcM)
*
* Also note that the d dependence is not in the ionic potential anymore;
* it is in the data table read in. Using d = d1 with a table generated for
* d = d2 will produce rubbish.

        function VzzSh5Im(r, costheta, xRV, mR, xCV, mC, 
     &                    VionM, dVxcM, xcFlag)
        implicit none
        double precision VzzSh5Im
        double precision r, costheta
        integer          mR, mC
        double precision xRV(1:mR), xCV(1:mC)
        double precision VionM(1:mR, 1:mC), dVxcM(1:mR, 1:mC)
        logical          xcFlag            ! include dVxc ?

        logical          equid             ! is data spaced equidistantly ?
        parameter       (equid = .false.)  ! nope.
        double precision Vion, dVxc

        integer          iRlo, iClo        ! index of last match
        save             iRlo, iClo        ! store between calls, important!

        external PotSRMLDF

*       ----------------------------------------------------------------------
*       The following 2 lines are a kludge.
*       There should be another way, possibly putting
*       all mR, mC, XRV, ... iRlo, iClo in a common block and saving this.
*       So far we ahve to test for a first call, where iRlo, iClo may be
*       uninitialized, hence -- garbage -- and mess up the index search,
*       causing NaNs. This should work now, but a common block may be faster.
*       ----------------------------------------------------------------------
        if ((iRlo.lt.1).or.(iRlo.gt.mR)) iRlo = 1
        if ((iClo.lt.1).or.(iClo.gt.mC)) iClo = 1

          call PotSRMLDF(r, costheta, xRV, mR, equid, xCV, mC, equid,
     &                   VionM, dVxcM, Vion, dVxc, iRlo, iClo)

          if (xcFlag.eqv..true.) then      ! include dVxc
            VzzSh5Im = Vion + dVxc
          else
            VzzSh5Im = Vion                ! don't
          endif

        return
        end
#line 2007
        function Vtot5(r, costheta, d, z0, lambda, u0, zJell,
     &                 xRV, mR, xCV, mC, VionM, dVxcM, xcFlag)
        implicit none
        double precision Vtot5
        double precision r, costheta, d, z0, lambda, u0, zJell
        integer          mR, mC
        double precision xRV(1:mR), xCV(1:mC)
        double precision VionM(1:mR, 1:mC), dVxcM(1:mR, 1:mC)
        double precision Vion
        logical          xcFlag           ! if TRUE, include dVxc

        double precision tmp, z

        double precision VzzSh5Im, VelSh
        external         VzzSh5Im, VelSh

        z    =   r*costheta

        Vion = VzzSh5Im(r, costheta, xRV, mR, xCV, mC,
     &                  VionM, dVxcM, xcFlag)
        tmp   = - Vion + VelSh(z-d,-z0,lambda,u0)
*       tmp   = - Vion 

        Vtot5 = tmp
        return
        end
#line 2035
C And, finally, to fit in the code, add -1/r for the ion and

      function VsurfMe05(xr, xb, r, z0, lambda, u0, zJell,
     &                   xRV, mR, xCV, mC, VionM, dVxcM, xcFlag)
      implicit none
      double precision VsurfMe05
      double precision xr, xb, r, z0, lambda, u0, zJell
      integer          mR, mC
      double precision xRV(1:mR), xCV(1:mC)
      double precision VionM(1:mR, 1:mC), dVxcM(1:mR, 1:mC)
      logical          xcFlag            ! .TRUE. to include dVxc

      double precision Vtot5
      external         Vtot5
#line 2051
        VsurfMe05 = -1.00d0/xr
     &              + Vtot5(xr, xb, r, z0, lambda, u0, zJell,
     &                      xRV, mR, xCV, mC, VionM, dVxcM, xcFlag)
c       VsurfMe05 =   Vtot5(xr, xb, r, z0, lambda, u0, zJell,
c    &                      xRV, mR, xCV, mC, VionM, dVxcM, xcFlag)
        return
      end

C This is the last line of my surface potential VsurfMe05
* It may be worthwhile putting the whole VionM, dVxcM stuff etc in a
* common block instead of passing them around like old rolls.
#line 2065
***************************************************************************
* end of file vsurflib.f
***************************************************************************
C Finito Italia!

C Comments from Xiazhou Yang: The code was tested by the screened Coulomb
C interaction (analytic) and the non-analytic image interaction. For the
C analytic potential, the results are the same as those of the rotation
C of potential. For the non-analytic potential, the results are about
C 10% different from the corresponding results of the rotation
C of potential.

C (!) Most editors on PCs produce a ^Z control character at the last
C character of the file to mark its end. This is invisible in DOS but
C when uploaded to cms it causes a last line containing only a "
C (quotation mark), which makes the compiler complain.
C So, this needs to be deleted, of course.
C It also chokes on tabs (^I) which need to be changed to spaces.
#line 2090
* ====================================================================
