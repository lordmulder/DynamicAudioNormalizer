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
# Rules
##############################################################################

BUILD_PROJECTS = DynamicAudioNormalizerAPI DynamicAudioNormalizerCLI
CLEAN_PROJECTS = $(addprefix Clean,$(BUILD_PROJECTS))

.PHONY: all clean distclean $(BUILD_PROJECTS) $(CLEAN_PROJECTS)

all: $(BUILD_PROJECTS)
	@$(ECHO) "\n\e[1;32mComplete.\e[0m\n"

clean: $(CLEAN_PROJECTS)
	@$(ECHO) "\n\e[1;32mComplete.\e[0m\n"

#-------------------------------------------------------------
# Clean
#-------------------------------------------------------------

$(CLEAN_PROJECTS):
	@$(ECHO) "\n\e[1;31m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;31mClean: $(patsubst Clean%,%,$@)\e[0m"
	@$(ECHO) "\e[1;31m-----------------------------------------------------------------------------\n\e[0m"
	make -C ./$(patsubst Clean%,%,$@) clean

#-------------------------------------------------------------
# Build
#-------------------------------------------------------------

$(BUILD_PROJECTS):
	@$(ECHO) "\n\e[1;34m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;34mBuild: $@\e[0m"
	@$(ECHO) "\e[1;34m-----------------------------------------------------------------------------\n\e[0m"
	make -C ./$@
