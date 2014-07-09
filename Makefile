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
LIBRARY_NAME := DynamicAudioNormalizerAPI
PROGRAM_NAME := DynamicAudioNormalizerCLI
BUILD_DATE   := $(shell date -Idate)
BUILD_TIME   := $(shell date +%H:%M:%S)
BUILD_TAG    := $(addprefix /tmp/,$(shell echo $$RANDOM$$RANDOM$$RANDOM))
TARGET_PATH  := ./bin/$(BUILD_DATE)
OUTPUT_FILE  := $(realpath ./bin/DynamicAudioNormalizer.$(BUILD_DATE).zip)

##############################################################################
# Rules
##############################################################################

BUILD_PROJECTS = DynamicAudioNormalizerShared DynamicAudioNormalizerAPI DynamicAudioNormalizerCLI
CLEAN_PROJECTS = $(addprefix Clean,$(BUILD_PROJECTS))

.PHONY: all clean $(BUILD_PROJECTS) $(CLEAN_PROJECTS) DeployBinaries CreateTagFile

all: $(BUILD_PROJECTS) DeployBinaries
	@$(ECHO) "\n\e[1;32mComplete.\e[0m\n"

clean: $(CLEAN_PROJECTS) CleanBinaries
	@$(ECHO) "\n\e[1;32mComplete.\e[0m\n"

#-------------------------------------------------------------
# Clean
#-------------------------------------------------------------

$(CLEAN_PROJECTS):
	@$(ECHO) "\n\e[1;31m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;31mClean: $(patsubst Clean%,%,$@)\e[0m"
	@$(ECHO) "\e[1;31m-----------------------------------------------------------------------------\n\e[0m"
	make -C ./$(patsubst Clean%,%,$@) clean

CleanBinaries:
	@$(ECHO) "\n\e[1;31m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;31mClean\e[0m"
	@$(ECHO) "\e[1;31m-----------------------------------------------------------------------------\n\e[0m"
	rm -rfv ./bin

#-------------------------------------------------------------
# Build
#-------------------------------------------------------------

$(BUILD_PROJECTS):
	@$(ECHO) "\n\e[1;34m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;34mBuild: $@\e[0m"
	@$(ECHO) "\e[1;34m-----------------------------------------------------------------------------\n\e[0m"
	make -C ./$@

#-------------------------------------------------------------
# Deploy
#-------------------------------------------------------------

DeployBinaries: CreateTagFile
	@$(ECHO) "\n\e[1;34m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;34mDeploy\e[0m"
	@$(ECHO) "\e[1;34m-----------------------------------------------------------------------------\n\e[0m"
	rm -rf $(TARGET_PATH)
	mkdir -p $(TARGET_PATH)/img
	mv -f $(BUILD_TAG) $(TARGET_PATH)/BUILD_TAG
	cp $(PROGRAM_NAME)/bin/$(PROGRAM_NAME) $(TARGET_PATH)
	cp $(LIBRARY_NAME)/lib/lib$(LIBRARY_NAME)-$(API_VERSION).so $(TARGET_PATH)
	cp ./LICENSE.html $(TARGET_PATH)
	pandoc --from markdown_github+header_attributes --to html5 --standalone -H ./img/Style.inc ./README.md --output $(TARGET_PATH)/README.html
	cp ./img/*.png $(TARGET_PATH)/img
	rm -f $(OUTPUT_FILE)
	pushd $(TARGET_PATH) && zip -r -9 -z $(OUTPUT_FILE) * < BUILD_TAG && popd

CreateTagFile:
	@$(ECHO) "\n\e[1;34m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;34mBuild Tag\e[0m"
	@$(ECHO) "\e[1;34m-----------------------------------------------------------------------------\n\e[0m"
	echo "Dynamic Audio Normalizer" > $(BUILD_TAG)
	echo "Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>" >> $(BUILD_TAG)
	echo "" >> $(BUILD_TAG)
	echo "Built on $(BUILD_DATE), at $(BUILD_TIME)" >> $(BUILD_TAG)
	g++ --version | head -n1 | sed 's/^/Compiler version: /' >> $(BUILD_TAG)
	echo "" >> $(BUILD_TAG)
	echo "This program is free software; you can redistribute it and/or modify" >> $(BUILD_TAG)
	echo "it under the terms of the GNU General Public License as published by" >> $(BUILD_TAG)
	echo "the Free Software Foundation; either version 2 of the License, or" >> $(BUILD_TAG)
	echo "(at your option) any later version." >> $(BUILD_TAG)

