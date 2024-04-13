#----------------------------------------------------------------------------
#                    DMX Sound System API Makefile
#----------------------------------------------------------------------------

CC = wcc386p		# name the compiler
MODEL = f		# flat
ASM = tasm

!ifeq %version debug
!   define CFLAGS_M -od -s -d2 -dDEBUG 
!   define AFLAGS_M -zi -dDEBUG
!else ifeq %version beta
!   define CFLAGS_M -oraixltf+ -d1+ -dBETA
!   define AFLAGS_M -zi
!else ifeq %version production
!   define CFLAGS_M -oraixltf+ -dPROD
!   define AFLAGS_M
!else
!   error environment var 'version' not set to <debug,beta,production>
!endif

CFLAGS = -m$(MODEL) -zq -zp1 -5r -ei $(CFLAGS_M)
FFLAGS = -m$(MODEL) -noterm
AFLAGS = -mx $(AFLAGS_M)

.EXTENSIONS:
.EXTENSIONS: .exe .rex .lib .obj .asm .c .h

.BEFORE
	@set INCLUDE=.;$(%watcom)\h;..\inc
	@set LIB=$(%watcom)\lib386;$(%lib)
	@set DOS4G=QUIET

.AFTER
	#flush

# !include dmx.mif

# explicit rules

all :	test.exe .symbolic
	@%null

test.exe: test.obj ..\lib\dmx.lib
	%make test.lnk
	 wlink @test.lnk

test.lnk : test.obj ..\lib\dmx.lib
        %create $^@
        %append $^@ NAME $^&
        %append $^@ OPT STACK=8192
	%append $^@ OPT CASEEXACT
        %append $^@ DEBUG all
        %append $^@ SYSTEM dos4g
	%append $^@ FILE test.obj
	%append $^@ LIBRARY ..\lib\dmx.lib


# implicit rules

.c.obj:
	$(CC) $(CFLAGS) $[*

.asm.obj :
	$(ASM) $(AFLAGS) $^&;
