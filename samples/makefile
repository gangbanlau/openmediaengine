PJSIP_SRC_PATH = /home/gang/pjsip/src/pjproject-2.0-alpha2

# google gflags
GOOGLE_GFLAGS_PATH = /home/gang/google/install

include $(PJSIP_SRC_PATH)/build.mak
include $(PJDIR)/build/common.mak

RULES_MAK = $(PJDIR)/build/rules.mak

include $(PJSIP_SRC_PATH)/build/common.mak

OBJDIR := build
SRCDIR := .
BINDIR := bin

SDKDIR = ../sdk

###############################################################################

export _CFLAGS = -Wall -pipe -g -I$(SDKDIR)/include $(PJ_CFLAGS) $(CFLAGS)
export _CXXFLAGS = -Wall -pipe -g -I$(SDKDIR)/include $(PJ_CXXFLAGS) $(CXXFLAGS) -I$(GOOGLE_GFLAGS_PATH)/include
export _LDFLAGS = $(SDKDIR)/build/libopenmediaengine.a $(PJ_LDFLAGS) $(PJ_LDLIBS) $(LDFLAGS) -L$(GOOGLE_GFLAGS_PATH)/lib -lgflags

CC=gcc
CXX=g++

SAMPLES = streamutil confbridge

EXES := $(foreach file, $(SAMPLES), $(BINDIR)/$(file)$(HOST_EXE))

all: $(OBJDIR) $(EXES)

$(BINDIR)/%$(HOST_EXE): $(OBJDIR)/%$(OBJEXT) $(PJ_LIB_FILES)
	$(CXX) $(LDOUT)$(subst /,$(HOST_PSEP),$@) \
		$(subst /,$(HOST_PSEP),$<) \
		$(_LDFLAGS)

$(OBJDIR)/%$(OBJEXT): $(SRCDIR)/%.cpp
	$(CXX) -c $(_CXXFLAGS) \
      $(CC_OUT)$(subst /,$(HOST_PSEP),$@) \
      $(subst /,$(HOST_PSEP),$<) 

$(OBJDIR):
	$(subst @@,$(subst /,$(HOST_PSEP),$@),$(HOST_MKDIR)) 

depend:

clean:
	$(subst @@,$(subst /,$(HOST_PSEP),$(OBJDIR)/*),$(HOST_RMR))
	$(subst @@,$(subst /,$(HOST_PSEP),$(OBJDIR)),$(HOST_RMDIR))
	$(subst @@,$(EXES),$(HOST_RM))
