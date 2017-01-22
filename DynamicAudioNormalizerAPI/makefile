##############################################################################
# Dynamic Audio Normalizer - Audio Processing Library
# Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# http://www.gnu.org/licenses/lgpl-2.1.txt
##############################################################################

ECHO=echo -e
SHELL=/bin/bash

JDK_PATH ?= $(shell update-java-alternatives -l 2>/dev/null | grep -oE '[^[:space:]]+$$' || echo "/usr/lib/jvm/default")

##############################################################################
# Constants
##############################################################################

LIBRARY_NAME := libDynamicAudioNormalizerAPI

ifndef API_VERSION
   $(error API_VERSION variable is not set!)
endif

##############################################################################
# JDK Checks
##############################################################################

ifneq ($(MAKECMDGOALS),clean)
   ifneq ($(ENABLE_JNI),false)
      JDK_CHECK := $(shell [ -f $(JDK_PATH)/include/jni.h ] && echo true || echo false)
      ifneq ($(JDK_CHECK),true)
         $(error File "include/jni.h" not found in JDK_PATH path!)
      endif
   endif
endif

##############################################################################
# Flags
##############################################################################

DEBUG_BUILD ?= 0
MARCH ?= native

ifeq ($(DEBUG_BUILD), 1)
   CONFIG_NAME = Debug
   CXXFLAGS = -g -O0 -D_DEBUG
else
   CONFIG_NAME = Release
   CXXFLAGS = -O3 -Wall -ffast-math -mfpmath=sse -msse -fomit-frame-pointer -fno-tree-vectorize -DNDEBUG -march=$(MARCH)
endif

ifneq ($(findstring MINGW,$(shell uname -s)),MINGW)
   CXXFLAGS += -fPIC
endif

CXXFLAGS += -std=gnu++11
CXXFLAGS += -fvisibility=hidden
CXXFLAGS += -DMDYNAMICAUDIONORMALIZER_EXPORTS

CXXFLAGS += -I./src
CXXFLAGS += -I./include
CXXFLAGS += -I../DynamicAudioNormalizerShared/include
CXXFLAGS += -I../DynamicAudioNormalizerJNI/include

ifneq ($(ENABLE_JNI),false)
   CXXFLAGS += -I$(JDK_PATH)/include
   ifeq ($(findstring MINGW,$(shell uname -s)),MINGW)
      CXXFLAGS += -I$(JDK_PATH)/include/win32
   else
      CXXFLAGS += -I$(JDK_PATH)/include/linux
   endif
else
   CXXFLAGS += -DNO_JAVA_SUPPORT
endif

##############################################################################
# File Names
##############################################################################

ifeq ($(findstring MINGW,$(shell uname -s)),MINGW)
   SO_EXT = dll
   LDXFLAGS += -Wl,--out-implib,$@.a
else
   SO_EXT = so
endif

SOURCES_CPP = $(wildcard src/*.cpp)
SOURCES_OBJ = $(patsubst %.cpp,%.o,$(SOURCES_CPP))

LIBRARY_OUT = lib/$(LIBRARY_NAME)-$(API_VERSION)
LIBRARY_BIN = $(LIBRARY_OUT).$(SO_EXT)
LIBRARY_DBG = $(LIBRARY_OUT)-DBG.$(SO_EXT)

##############################################################################
# Rules
##############################################################################

.PHONY: all clean

all: $(LIBRARY_DBG) $(LIBRARY_BIN)

#-------------------------------------------------------------
# Link & Strip
#-------------------------------------------------------------

$(LIBRARY_BIN): $(SOURCES_OBJ)
	@$(ECHO) "\e[1;36m[STR] $@\e[0m"
	@mkdir -p $(dir $@)
	g++ -shared -g0 -o $@ $+ $(LDXFLAGS)
	strip --strip-unneeded $@

$(LIBRARY_DBG): $(SOURCES_OBJ)
	@$(ECHO) "\e[1;36m[LNK] $@\e[0m"
	@mkdir -p $(dir $@)
	g++ -shared -o $@ $+ $(LDXFLAGS)

#-------------------------------------------------------------
# Compile
#-------------------------------------------------------------

%.o: %.cpp
	@$(ECHO) "\e[1;36m[CXX] $<\e[0m"
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -c $< -o $@

#-------------------------------------------------------------
# Clean
#-------------------------------------------------------------

clean:
	rm -fv $(SOURCES_OBJ)
	rm -rfv ./lib

