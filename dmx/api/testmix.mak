#----------------------------------------------------------------------------
#                    DMX Sound System API Makefile
#----------------------------------------------------------------------------

CC = wcc386p		# name the compiler
MODEL = f		# flat
ASM = tasm

!ifeq %version debug
!   define CFLAGS_M -od -d2 -dDEBUG 
!   define AFLAGS_M -zi -dDEBUG
!else ifeq %version beta
#!   define CFLAGS_M -oraixltf+ -d1+ -dBETA
!   define CFLAGS_M -oxt -d1+ -dBETA
!   define AFLAGS_M -zi
!else ifeq %version production
!   define CFLAGS_M -oxt -4r -dPROD
!   define AFLAGS_M
!else
!   error environment var 'version' not set to <debug,beta,production>
!endif

# !   define CFLAGS_M -oa -ox -w4 -oraixltf+ -dPROD

CFLAGS = -m$(MODEL) -zq -zp1 -s -zgp $(CFLAGS_M)
FFLAGS = -m$(MODEL) -noterm
AFLAGS = -ml $(AFLAGS_M)

.EXTENSIONS:
.EXTENSIONS: .exe .rex .lib .obj .asm .c .h

.BEFORE
	@set INCLUDE=.;$(%watcom)\h;..\inc
	@set LIB=$(%watcom)\lib386;$(%lib)
	@set DOS4G=QUIET

.AFTER
	#flush

!include dmx.mif

# explicit rules

all :	testmix.exe .symbolic
	@%null

testmix.exe: testmix.obj pztimer.obj mixers.obj
	%make testmix.lnk    
	wlink @testmix.lnk

testmix.lnk: testmix.mak
        %create $^@
        %append $^@ NAME $^&
        %append $^@ OPT STACK=8192
	%append $^@ OPT CASEEXACT
        %append $^@ DEBUG all
        %append $^@ SYSTEM dos4g
	%append $^@ FILE testmix.obj
	%append $^@ FILE pztimer.obj
	%append $^@ FILE mixers.obj

mixers.obj:	mixers.asm 	clip.inc

pztimer.obj:	pztimer.asm

# implicit rules

.c.obj:
	$(CC) $(CFLAGS) $[*

.asm.obj :
	$(ASM) $(AFLAGS) $^&;
