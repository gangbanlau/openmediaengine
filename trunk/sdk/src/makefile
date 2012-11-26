#########################################################
#
# Makefile for GNU/Linux GCC
#
#########################################################

#PJSIP_SRC_PATH = /home/gang/pjsip/src/pjsip_silk/icc/pjproject-2.0-alpha2.silk/
#PJSIP_SRC_PATH = /home/gang/pjsip/src/pjproject-2.0-alpha2.silk/
PJSIP_SRC_PATH = /home/gang/pjsip/src/pjsip_opus/pjproject-2.0.silk.opus

include $(PJSIP_SRC_PATH)/build.mak
include $(PJDIR)/build/common.mak

RULES_MAK = $(PJDIR)/build/rules.mak

include $(PJSIP_SRC_PATH)/build/common.mak

OBJ_DIR = ../build
SDK_INC_DIR = ../include

export _CFLAGS = -D__OPENMEDIAENGINE__ -O3 -g -fomit-frame-pointer -fno-strict-aliasing -Wall -DNDEBUG -D_REENTRANT -I$(SDK_INC_DIR) $(PJ_CFLAGS) $(CFLAGS)
export _CXXFLAGS = -D__OPENMEDIAENGINE__ -O3 -g -fomit-frame-pointer -fno-strict-aliasing -Wall -DNDEBUG -D_REENTRANT -I$(SDK_INC_DIR) $(PJ_CXXFLAGS)
export _LDFLAGS = $(PJ_LDFLAGS) $(PJ_LDLIBS) $(LDFLAGS)

CC = $CC
#CXX = icpc

TARGET = libopenmediaengine.a

%.o: %.c
	$(CC) $(_CFLAGS) -c $< -o $(OBJ_DIR)/$@

%.o: %.cpp
	$(CXX) $(_CXXFLAGS) -c $< -o $(OBJ_DIR)/$@

SOURCES = $(wildcard *.c *.cpp)
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

$(TARGET) : $(OBJS)
	ar rcs $(OBJ_DIR)/$(TARGET) `for X in $(OBJS) ; do echo "$(OBJ_DIR)/$$X"; done`

all: $(TARGET)

clean:
	rm -rf $(OBJ_DIR)/*.o $(OBJ_DIR)/$(TARGET)
