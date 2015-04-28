##############################################################################
# Compiler and flags
CC= g++
CCOMP= gcc
CFLAGS= -O3

USEBLAS= True

# NETCDF library directories
NETCDF_INCLUDEDIR=$(NETCDF_DIR)/include
NETCDF_LIBDIR=$(NETCDF_DIR)/lib

# Library files to include
LDFILES= -lnetcdf -lnetcdf_c++ -L. 

##############################################################################
# DO NOT MODIFY BELOW THIS LINE
##############################################################################

# Local files
######################################
#ADDED MY FILES HERE!
######################################
CLIVAR_FILES = CLIVAR_blocks.cpp Announce.cpp NetCDFUtilities.cpp CLIVAR_block_utilities.cpp interpolate.cpp TimeObj.cpp
CLIVAR_CFILES =

BLOCK_FILES = moving_avg.cpp Announce.cpp NetCDFUtilities.cpp TimeObj.cpp  CLIVAR_block_utilities.cpp 
BLOCK_CFILES = 

DEV_FILES = devIPV.cpp Announce.cpp NetCDFUtilities.cpp CLIVAR_block_utilities.cpp TimeObj.cpp
DEV_CFILES = 
######################################

STITCHNODES_FILES= StitchNodes.cpp Announce.cpp
STITCHNODES_CFILES= kdtree.c

STITCHBLOBS_FILES= StitchBlobs.cpp Announce.cpp NetCDFUtilities.cpp
STITCHBLOBS_CFILES= kdtree.c

DETECTCYCLONES_FILES= DetectCyclones.cpp Announce.cpp TimeObj.cpp
DETECTCYCLONES_CFILES= kdtree.c

DENSITYNODES_FILES= DensityNodes.cpp Announce.cpp
DENSITYNODES_CFILES=

# Load system-specific defaults
CFLAGS+= -I$(NETCDF_INCLUDEDIR)
LDFLAGS+= -L$(NETCDF_LIBDIR)

include Make.defs

##
## Build instructions
##
all: StitchNodes StitchBlobs DetectCyclones DensityNodes CLIVAR_blocks AvgIPV DevIPV

StitchNodes: $(STITCHNODES_FILES:%.cpp=$(BUILDDIR)/%.o) $(STITCHNODES_CFILES:%.c=$(BUILDDIR)/%.o)
	$(CC) $(LDFLAGS) -o $@ $(STITCHNODES_FILES:%.cpp=$(BUILDDIR)/%.o) $(STITCHNODES_CFILES:%.c=$(BUILDDIR)/%.o) $(LDFILES)

StitchBlobs: $(STITCHBLOBS_FILES:%.cpp=$(BUILDDIR)/%.o) $(STITCHBLOBS_CFILES:%.c=$(BUILDDIR)/%.o)
	$(CC) $(LDFLAGS) -o $@ $(STITCHBLOBS_FILES:%.cpp=$(BUILDDIR)/%.o) $(STITCHBLOBS_CFILES:%.c=$(BUILDDIR)/%.o) $(LDFILES)

DetectCyclones: $(DETECTCYCLONES_FILES:%.cpp=$(BUILDDIR)/%.o) $(DETECTCYCLONES_CFILES:%.c=$(BUILDDIR)/%.o)
	$(CC) $(LDFLAGS) -o $@ $(DETECTCYCLONES_FILES:%.cpp=$(BUILDDIR)/%.o) $(DETECTCYCLONES_CFILES:%.c=$(BUILDDIR)/%.o) $(LDFILES)

DensityNodes: $(DENSITYNODES_FILES:%.cpp=$(BUILDDIR)/%.o) $(DENSITYNODES_CFILES:%.c=$(BUILDDIR)/%.o)
	$(CC) $(LDFLAGS) -o $@ $(DENSITYNODES_FILES:%.cpp=$(BUILDDIR)/%.o) $(DENSITYNODES_CFILES:%.c=$(BUILDDIR)/%.o) $(LDFILES)

CLIVAR_blocks: $(CLIVAR_FILES:%.cpp=$(BUILDDIR)/%.o) $(CLIVAR_CFILES:%.c=$(BUILDDIR)/%.o)
	$(CC) $(LDFLAGS) -o $@ $(CLIVAR_FILES:%.cpp=$(BUILDDIR)/%.o) $(CLIVAR_CFILES:%.c=$(BUILDDIR)/%.o) $(LDFILES)

AvgIPV: $(BLOCK_FILES:%.cpp=$(BUILDDIR)/%.o) $(BLOCK_CFILES:%.c=$(BUILDDIR)/%.o)
	$(CC) $(LDFLAGS) -o $@ $(BLOCK_FILES:%.cpp=$(BUILDDIR)/%.o) $(BLOCK_CFILES:%.c=$(BUILDDIR)/%.o) $(LDFILES)
DevIPV: $(DEV_FILES:%.cpp=$(BUILDDIR)/%.o) $(DEV_CFILES:%.c=$(BUILDDIR)/%.o)
	$(CC) $(LDFLAGS) -o $@ $(DEV_FILES:%.cpp=$(BUILDDIR)/%.o) $(DEV_CFILES:%.c=$(BUILDDIR)/%.o) $(LDFILES)

## Clean
##
clean:
	rm -f StitchNodes StitchBlobs DetectCyclones DensityNodes CLIVAR_blocks AvgIPV DevIPV
	rm -rf $(DEPDIR)
	rm -rf $(BUILDDIR)

##
## Include dependencies
##
include $(FILES:%.cpp=$(DEPDIR)/%.d)
include $(FILES:%.c=$(DEPDIR)/%.d)

# DO NOT DELETE

