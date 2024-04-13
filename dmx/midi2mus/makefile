#
# PVCS makefile for MIDI2MUS
#
.MEMSWAP CL LINK MAKE

#
# Do no echo command issued to stdout
#
.SILENT

#
# Do not use any rules from any "bulitins" files
#
.SUFFIXES

#
# Precious files do not get deleted when using an implied rule
#
.PRECIOUS .c
.PRECIOUS .h

#
# file locations
#
OBJ_DIR = obj

.PATH.c         = 
.PATH.h         = 
.PATH.obj       = $(OBJ_DIR)
.PATH.exe       = ..\bin

.INIT:
	@echo Initializing Environment...
	%setenv	LIB=$(MSC)\LIB;$(LIB)
	%setenv INCLUDE=$(MSC)\INCLUDE;$(INCLUDE)
#
# Compiler Options
#

CFLAGS_G = $(GCOMP_FLAGS) /nologo /IF:\c7\include /AL /G2 /Zp /AL /W2 /Fo$(OBJ_DIR)\ /c
CFLAGS_D = $(DCOMP_FLAGS) /Zid /Od
CFLAGS_R = $(RCOMP_FLAGS) /Gs /Ox

#
# Linker Options
#

LFLAGS_G = $(GLINK_FLAGS) /NOE /STACK:4096
LFLAGS_D = $(DLINK_FLAGS) /CO 
LFLAGS_R = $(RLINK_FLAGS) /EXEPACK

#
# target creation options
#

%if %defined(DEBUG)
	COMPILE_OPTS = $(CFLAGS_G) $(CFLAGS_D)
	LINK_OPTS    = $(LFLAGS_G) $(LFLAGS_D)			  
%else
	COMPILE_OPTS = $(CFLAGS_G) $(CFLAGS_R)
	LINK_OPTS    = $(LFLAGS_G) $(LFLAGS_R)			  
%endif

#
# Implied Rules
#

.c.obj:
   @echo Compiling...$<
        cl $(COMPILE_OPTS) $<
#

.obj.exe:
   @echo Exe Linking...$<
	link $(LINK_OPTS) <@<
$[s, "+ \n", $[m, "*.OBJ",$** ]]
$*.exe
NUL 
$(CRN_LIBS) $[s, "+ \n", $[m, "*.LIB",$** ]]

<

#
# Targets
#
ALL:		midi2mus.exe

midi2mus.exe:	midi2mus.obj

