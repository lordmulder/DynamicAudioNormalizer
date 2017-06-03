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

PYTHON_INC ?= $(shell python3 -c "from distutils import sysconfig; print(sysconfig.get_python_inc())" || echo "/usr/include/python3.5")

##############################################################################
# Constants
##############################################################################

LIBRARY_NAME := DynamicAudioNormalizerPYD

ifndef API_VERSION
   $(error API_VERSION variable is not set!)
endif

##############################################################################
# Python Checks
##############################################################################

ifneq ($(MAKECMDGOALS),clean)
   PY_CHECK := $(shell [ -f $(PYTHON_INC)/Python.h ] && echo true || echo false)
   ifneq ($(PY_CHECK),true)
      $(error File "Python.h" not found in PYTHON_INC path!)
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

CXXFLAGS += -I./src
CXXFLAGS += -I../DynamicAudioNormalizerAPI/include
CXXFLAGS += -I../DynamicAudioNormalizerShared/include
CXXFLAGS += -I$(PYTHON_INC)

ifneq ($(findstring MINGW,$(shell uname -s)),MINGW)
   LDXFLAGS += -Wl,-rpath,'$$ORIGIN'
endif

LDXFLAGS += -L../DynamicAudioNormalizerAPI/lib
LDXFLAGS += -lDynamicAudioNormalizerAPI-$(API_VERSION)
ifeq ($(shell uname), Darwin)
	PYTHON_LIB := $(shell python3 -c "from distutils import sysconfig; print(sysconfig.get_config_var('LIBDIR'))")
	PYTHON_VERSION := $(shell python3 -c "from distutils import sysconfig; print(sysconfig.get_config_var('VERSION'))")
	LDXFLAGS += -lpython$(PYTHON_VERSION) -L$(PYTHON_LIB)
endif

##############################################################################
# File Names
##############################################################################

ifeq ($(findstring MINGW,$(shell uname -s)),MINGW)
   SO_EXT = pyd
   LDXFLAGS += -Wl,--out-implib,$@.a
   SHAREDFLAG = -shared
else ifeq ($(shell uname), Darwin)
   SO_EXT = so
#   SHAREDFLAG = -dynamiclib
   SHAREDFLAG = -shared
else
   SO_EXT = so
   SHAREDFLAG = -shared
endif

SOURCES_CPP = $(wildcard src/*.cpp)
SOURCES_OBJ = $(patsubst %.cpp,%.o,$(SOURCES_CPP))

LIBRARY_OUT = lib/$(LIBRARY_NAME)
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
	g++ $(SHAREDFLAG) -g0 -o $@ $+ $(LDXFLAGS)
ifeq ($(shell uname), Darwin)
	install_name_tool -id "@rpath/$(notdir $@)" $@
	strip -u -r -x $@
else
	strip --strip-unneeded $@
endif

$(LIBRARY_DBG): $(SOURCES_OBJ)
	@$(ECHO) "\e[1;36m[LNK] $@\e[0m"
	@mkdir -p $(dir $@)
	g++ $(SHAREDFLAG) -o $@ $+ $(LDXFLAGS)
ifeq ($(shell uname), Darwin)
	install_name_tool -id "@rpath/$(notdir $@)" $@
endif

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

