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

MOC ?= moc
QTDIR ?= /usr/include/qt4

PROGRAM_NAME := DynamicAudioNormalizerGUI

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

CXXFLAGS += -I./src
CXXFLAGS += -I./include

CXXFLAGS += -I$(QTDIR)
CXXFLAGS += -I$(QTDIR)/QtGui
CXXFLAGS += -I$(QTDIR)/QtCore

#Externel libraries
LDXFLAGS += -lQtCore -lQtGui

##############################################################################
# Rules
##############################################################################

SOURCES_HDR = $(wildcard src/*.h) $(wildcard src/3rd_party/*.h)
SOURCES_MOC = $(patsubst %.h,%_MOC.cpp,$(SOURCES_HDR))
SOURCES_CPP = $(wildcard src/*.cpp) $(wildcard src/3rd_party/*.cpp) $(SOURCES_MOC)
SOURCES_OBJ = $(patsubst %.cpp,%.o,$(SOURCES_CPP))

PROGRAM_OUT = bin/$(PROGRAM_NAME)
PROGRAM_BIN = $(PROGRAM_OUT)
PROGRAM_DBG = $(PROGRAM_OUT)-DBG

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

