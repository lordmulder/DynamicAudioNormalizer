##############################################################################
# Dynamic Audio Normalizer - Audio Processing Utility
# Copyright (c) 2014-2018 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# http://www.gnu.org/licenses/gpl-3.0.txt
##############################################################################

ECHO=echo -e
SHELL=/bin/bash

MOC ?= moc
QTDIR ?= /usr/include/qt4

##############################################################################
# Constants
##############################################################################

PROGRAM_NAME := DynamicAudioNormalizerGUI

ifndef API_VERSION
   $(error API_VERSION variable is not set!)
endif

##############################################################################
# Qt Checks
##############################################################################

ifneq ($(MAKECMDGOALS),clean)
   MOC_CHECK := $(findstring Qt Meta Object Compiler,$(shell $(MOC) -v 2>&1))
   ifneq ($(MOC_CHECK),Qt Meta Object Compiler)
      $(error Qt Meta Object Compiler not found. Please set MOC environment variable properly!)
   endif
   ifneq ($(notdir $(realpath $(QTDIR)/QtGui/QApplication)),QApplication)
      $(error File "QtGui/QApplication" not found in Qt include path. Please set QTDIR environment variable properly!)
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
   CXXFLAGS = -O3 -Wall -ffast-math -mfpmath=sse -msse -fomit-frame-pointer -fno-tree-vectorize -DNDEBUG -march=$(MARCH) -Wno-strict-overflow
endif

CXXFLAGS += -I./src
CXXFLAGS += -I../DynamicAudioNormalizerAPI/include

LDXFLAGS += -Wl,-rpath,'$$ORIGIN'
LDXFLAGS += -L../DynamicAudioNormalizerAPI/lib
LDXFLAGS += -lDynamicAudioNormalizerAPI-$(API_VERSION)

#Externel libraries
CXXFLAGS += -I$(QTDIR)
CXXFLAGS += -I$(QTDIR)/QtGui
CXXFLAGS += -I$(QTDIR)/QtCore

LDXFLAGS += -lQtCore -lQtGui

##############################################################################
# File Names
##############################################################################

ifeq ($(findstring MINGW,$(shell uname -s)),MINGW)
   PRG_EXT = exe
else
   PRG_EXT = bin
endif

SOURCES_HDR = $(wildcard src/*.h) $(wildcard src/3rd_party/*.h)
SOURCES_MOC = $(patsubst %.h,%_MOC.cpp,$(SOURCES_HDR))
SOURCES_CPP = $(wildcard src/*.cpp) $(wildcard src/3rd_party/*.cpp) $(SOURCES_MOC)
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

%_MOC.cpp: %.h
	@$(ECHO) "\e[1;36m[MOC] $<\e[0m"
	@mkdir -p $(dir $@)
	$(MOC) -o $@ $<

%.o: %.cpp
	@$(ECHO) "\e[1;36m[CXX] $<\e[0m"
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -c $< -o $@

#-------------------------------------------------------------
# Clean
#-------------------------------------------------------------

clean:
	rm -fv $(SOURCES_OBJ)
	rm -fv $(SOURCES_MOC)
	rm -rfv ./bin

