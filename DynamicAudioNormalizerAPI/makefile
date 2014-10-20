##############################################################################
# Dynamic Audio Normalizer - Audio Processing Library
# Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

JDK_PATH ?= $(shell update-java-alternatives -l | grep -oE '[^[:space:]]+$$')

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
   ifneq ($(notdir $(realpath $(JDK_PATH)/include/jni.h)),jni.h)
      $(error File "include/jni.h" not found in JDK path. Please set JDK_PATH environment variable properly!)
   endif
endif

##############################################################################
# Flags
##############################################################################

DEBUG_BUILD ?= 0

ifeq ($(DEBUG_BUILD), 1)
   CONFIG_NAME = Debug
   CXXFLAGS = -g -O0 -D_DEBUG
else
   CONFIG_NAME = Release
   CXXFLAGS = -O3 -Wall -ffast-math -mfpmath=sse -msse -fomit-frame-pointer -fno-tree-vectorize -DNDEBUG
   ifeq ($(shell getconf LONG_BIT),64)
      CXXFLAGS += -march=core2
   else
      CXXFLAGS += -march=pentium4
   endif
endif

CXXFLAGS += -std=gnu++11
CXXFLAGS += -fPIC
CXXFLAGS += -fvisibility=hidden
CXXFLAGS += -DMDYNAMICAUDIONORMALIZER_EXPORTS

CXXFLAGS += -I./src
CXXFLAGS += -I./include
CXXFLAGS += -I../DynamicAudioNormalizerShared/include
CXXFLAGS += -I../DynamicAudioNormalizerJNI/include
CXXFLAGS += -I$(JDK_PATH)/include
CXXFLAGS += -I$(JDK_PATH)/include/linux

##############################################################################
# Rules
##############################################################################

SOURCES_CPP = $(wildcard src/*.cpp)
SOURCES_OBJ = $(patsubst %.cpp,%.o,$(SOURCES_CPP))

LIBRARY_OUT = lib/$(LIBRARY_NAME)-$(API_VERSION)
LIBRARY_BIN = $(LIBRARY_OUT).so
LIBRARY_DBG = $(LIBRARY_OUT)-DBG.so

##############################################################################
# Rules
##############################################################################

.PHONY: all clean

all: $(LIBRARY_BIN)

#-------------------------------------------------------------
# Link & Strip
#-------------------------------------------------------------

$(LIBRARY_BIN): $(LIBRARY_DBG)
	@$(ECHO) "\e[1;36m[STR] $@\e[0m"
	@mkdir -p $(dir $@)
	strip --strip-unneeded -o $@ $<

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

