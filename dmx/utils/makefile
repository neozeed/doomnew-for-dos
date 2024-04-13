#----------------------------------------------------------------------------
#                    DMX Sound System API Makefile
#----------------------------------------------------------------------------

CC = wcc386p		# name the compiler
MODEL = f		# flat
ASM = tasm

!ifndef %version
!    define %version = production
!endif

!ifeq %version debug
!   define CFLAGS_M -od -s -d2 -zdp -dDEBUG
!   define AFLAGS_M -zi
!else
!   define CFLAGS_M -oiaxt
!   define AFLAGS_M
!endif

CFLAGS = -m$(MODEL) -zq -zp1 -ei $(CFLAGS_M) 
FFLAGS = -m$(MODEL) -noterm
AFLAGS = -mx $(AFLAGS_M)

.EXTENSIONS:
.EXTENSIONS: .exe .rex .lib .obj .asm .c .h

.BEFORE
	@set INCLUDE=.;$(%include);$(%watcom)\h
	@set LIB=$(%watcom)\lib386;$(%lib)
	@set DOS4G=QUIET

# explicit rules

all :	wav2dsp.exe .symbolic
	@%null

wav2dsp.exe: wav2dsp.obj
	%make wav2dsp.lnk
	wlink @wav2dsp.lnk
	copy wav2dsp.exe ..\bin

wav2dsp.lnk : wav2dsp.obj
        %create $^@
        %append $^@ NAME $^&
        %append $^@ OPT STACK=8192
        %append $^@ DEBUG all
        %append $^@ SYSTEM dos4g
	%append $^@ FILE wav2dsp.obj

# implicit rules

.c.obj:
	$(CC) $(CFLAGS) $[*

.asm.obj :
	$(ASM) $(AFLAGS) $^&;
