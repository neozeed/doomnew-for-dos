#----------------------------------------------------------------------------
#                    DMX Sound System API Makefile
#----------------------------------------------------------------------------

CC = wcc386		# name the compiler
MODEL = f		# flat
ASM = tasm

ARGS_STACK = -3s
ARGS_REGS  = -4r

version=production

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
# CFLAGS = -m$(MODEL) -zq -zp1 -s -zg $(CFLAGS_M)
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

all :	dmx.lib test.exe .symbolic
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

dmx.lib: $(objs)
	%make temp.lnk    
	wlib $^@ /n /b @temp.lnk
	copy $^@ ..\lib

temp.lnk: dmx.mif makefile 
        %create $^@
        for %i in ($(objs)) do %append $^@ -+%i
	%append $^@ lib\pawe32.lib

mixers.obj:	mixers.asm 	clip.inc

# implicit rules

.c.obj:
	$(CC) $(CFLAGS) $[*

.asm.obj :
	$(ASM) $(AFLAGS) $^&;
