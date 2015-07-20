##############################################################################
# Dynamic Audio Normalizer - Audio Processing Utility
# Copyright (c) 2015 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# http://www.gnu.org/licenses/gpl-2.0.txt
##############################################################################

ECHO=echo -e
SHELL=/bin/bash

##############################################################################
# Constants
##############################################################################

PROGRAM_NAME := DynamicAudioNormalizerCLI

ifndef API_VERSION
   $(error API_VERSION variable is not set!)
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

CXXFLAGS += -I./src
CXXFLAGS += -I./include
CXXFLAGS += -I../DynamicAudioNormalizerAPI/include
CXXFLAGS += -I../DynamicAudioNormalizerShared/include

ifeq ($(findstring MINGW,$(shell uname -s)),MINGW)
   CXXFLAGS += -D__USE_MINGW_FSEEK 
   CXXFLAGS += -I../../libsndfile/include
   LDXFLAGS += -L../../libsndfile/lib
endif

LDXFLAGS += -Wl,-rpath,'$$ORIGIN'
LDXFLAGS += -L../DynamicAudioNormalizerAPI/lib
LDXFLAGS += -lDynamicAudioNormalizerAPI-$(API_VERSION)

#Externel libraries
LDXFLAGS += -lsndfile

##############################################################################
# File Names
##############################################################################

ifeq ($(findstring MINGW,$(shell uname -s)),MINGW)
   PRG_EXT = exe
else
   PRG_EXT = bin
endif

SOURCES_CPP = $(wildcard src/*.cpp)
SOURCES_OBJ = $(patsubst %.cpp,%.o,$(SOURCES_CPP))

PROGRAM_OUT = bin/$(PROGRAM_NAME)
PROGRAM_BIN = $(PROGRAM_OUT).$(PRG_EXT)
PROGRAM_DBG = $(PROGRAM_OUT)-DBG.$(PRG_EXT)

##############################################################################
# Rules
##############################################################################

.PHONY: all clean

all: $(PROGRAM_BIN)

#-------------------------------------------------------------
# Link & Strip
#-------------------------------------------------------------

$(PROGRAM_BIN): $(PROGRAM_DBG)
	@$(ECHO) "\e[1;36m[STR] $@\e[0m"
	@mkdir -p $(dir $@)
	strip --strip-unneeded -o $@ $<

$(PROGRAM_DBG): $(SOURCES_OBJ)
	@$(ECHO) "\e[1;36m[LNK] $@\e[0m"
	@mkdir -p $(dir $@)
	g++ -o $@ $+ $(LDXFLAGS)

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
	rm -rfv ./bin

