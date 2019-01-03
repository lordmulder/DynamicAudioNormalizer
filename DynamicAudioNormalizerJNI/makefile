##############################################################################
# Dynamic Audio Normalizer - Audio Processing Library
# Copyright (c) 2014-2019 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

ANT ?= ant

##############################################################################
# Ant Checks
##############################################################################

ifneq ($(MAKECMDGOALS),clean)
   ANT_CHECK = $(findstring Apache Ant,$(shell $(ANT) -version 2>&1))
   ifneq ($(ANT_CHECK),Apache Ant)
      $(error Apache Ant not found. Please set ANT environment variable properly!)
   endif
endif

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

#-------------------------------------------------------------
# Compile
#-------------------------------------------------------------

all: build.xml
	@$(ECHO) "\e[1;36m[ANT] $<\e[0m"
	$(ANT) -buildfile $<

#-------------------------------------------------------------
# Clean
#-------------------------------------------------------------

clean: build.xml
	@$(ECHO) "\e[1;36m[ANT] $< clean\e[0m"
	$(ANT) -buildfile $< clean || echo "Ant clean failed!"
	rm -rfv ./bin
	rm -rfv ./out

