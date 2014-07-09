##############################################################################
# Dynamic Audio Normalizer
# Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version, but always including the *additional*
# restrictions defined in the "License.txt" file.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# http://www.gnu.org/licenses/gpl-2.0.txt
##############################################################################

ECHO=echo -e
SHELL=/bin/bash

##############################################################################
# Constants
##############################################################################

API_VERSION  := 1
LIBRARY_NAME := libDynamicAudioNormalizer

##############################################################################
# Flags
##############################################################################

DEBUG_BUILD ?= 0

ifeq ($(DEBUG_BUILD), 1)
   CONFIG_NAME = Debug
   CXXFLAGS = -g -O0 -D_DEBUG
else
   CONFIG_NAME = Release
   CXXFLAGS = -O3 -march=native -Wall -ffast-math -mfpmath=sse -msse -fomit-frame-pointer -fno-tree-vectorize -DNDEBUG
endif

CXXFLAGS += -fPIC
CXXFLAGS += -fvisibility=hidden
LDXFLAGS += -shared

CXXFLAGS += -I./src
CXXFLAGS += -I./include
CXXFLAGS += -I../DynamicAudioNormalizerShared/include

##############################################################################
# Rules
##############################################################################

SOURCES_CPP = $(wildcard src/*.cpp) $(wildcard ../DynamicAudioNormalizerShared/src/*.cpp)
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
	g++ $(LDXFLAGS) -o $@ $+

#-------------------------------------------------------------
# Compile
#-------------------------------------------------------------

%.o: %.cpp
	@$(ECHO) "\e[1;36m[CXX] $<\e[0m"
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -c $< -o $@

#-------------------------------------------------------------
# Compile
#-------------------------------------------------------------

clean:
	rm -fv $(SOURCES_OBJ)
	rm -fv $(LIBRARY_BIN)
	rm -fv $(LIBRARY_DBG)

