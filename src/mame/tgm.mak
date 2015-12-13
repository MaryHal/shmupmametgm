###########################################################################
#
#   tgm.mak
#
#   Small driver-specific example makefile
#	Use make SUBTARGET=tiny to build
#
#   Copyright Nicola Salmoria and the MAME Team.
#   Visit  http://mamedev.org for licensing and usage restrictions.
#
###########################################################################

MAMESRC = $(SRC)/mame
MAMEOBJ = $(OBJ)/mame

AUDIO = $(MAMEOBJ)/audio
DRIVERS = $(MAMEOBJ)/drivers
LAYOUT = $(MAMEOBJ)/layout
MACHINE = $(MAMEOBJ)/machine
VIDEO = $(MAMEOBJ)/video

OBJDIRS += \
	$(AUDIO) \
	$(DRIVERS) \
	$(LAYOUT) \
	$(MACHINE) \
	$(VIDEO) \



#-------------------------------------------------
# Specify all the CPU cores necessary for the
# drivers referenced in tgm.c.
#-------------------------------------------------

CPUS += Z80
CPUS += MCS48
CPUS += M680X0
CPUS += MIPS
CPUS += SH2
CPUS += MN10200
CPUS += I86


#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in tgm.c.
#-------------------------------------------------

SOUNDS += YM2610B
SOUNDS += YMF271
SOUNDS += YMF278B
SOUNDS += YMZ280B
SOUNDS += YM2608
SOUNDS += YM2203
SOUNDS += QSOUND
SOUNDS += PSXSPU
SOUNDS += ZSG2



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in tgm.c
#-------------------------------------------------

DRVLIBS = \
	$(MAMEOBJ)/tgm.o \
	$(DRIVERS)/zn.o $(MACHINE)/znsec.o \
	$(DRIVERS)/psikyosh.o $(VIDEO)/psikyosh.o \
	$(MACHINE)/psx.o $(VIDEO)/psx.o \
	$(AUDIO)/taitosnd.o \
	$(AUDIO)/taito_zm.o \

