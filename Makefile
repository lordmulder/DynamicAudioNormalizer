##############################################################################
# Dynamic Audio Normalizer
# Copyright (C) 2015 LoRd_MuldeR <MuldeR2@GMX.de>
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

MODE ?= full
PANDOC ?= pandoc

##############################################################################
# Constants
##############################################################################

LIBRARY_NAME := DynamicAudioNormalizerAPI
PROGRAM_NAME := DynamicAudioNormalizerCLI
LOGVIEW_NAME := DynamicAudioNormalizerGUI
JNIWRAP_NAME := DynamicAudioNormalizerJNI
PASWRAP_NAME := DynamicAudioNormalizerPAS
BUILD_DATE   := $(shell date -Idate)
BUILD_TIME   := $(shell date +%H:%M:%S)
BUILD_TAG    := $(addprefix /tmp/,$(shell echo $$RANDOM$$RANDOM$$RANDOM))
TARGET_PATH  := ./bin/$(BUILD_DATE)
OUTPUT_FILE  := $(abspath ./bin/DynamicAudioNormalizer.$(BUILD_DATE).tbz2)
PANDOC_FLAGS := markdown_github+pandoc_title_block+header_attributes+implicit_figures

# Version info
VER_MAJOR = $(shell sed -n 's/.*DYNAUDNORM_VERSION_MAJOR = \([0-9]*\).*/\1/p' < ./DynamicAudioNormalizerAPI/src/Version.cpp)
VER_MINOR = $(shell sed -n 's/.*DYNAUDNORM_VERSION_MINOR = \([0-9]*\).*/\1/p' < ./DynamicAudioNormalizerAPI/src/Version.cpp)
VER_PATCH = $(shell sed -n 's/.*DYNAUDNORM_VERSION_PATCH = \([0-9]*\).*/\1/p' < ./DynamicAudioNormalizerAPI/src/Version.cpp)

# API Version
export API_VERSION := $(shell sed -n 's/.*define MDYNAMICAUDIONORMALIZER_CORE \([0-9]*\).*/\1/p' < ./DynamicAudioNormalizerAPI/include/DynamicAudioNormalizer.h)

##############################################################################
# Projects
##############################################################################

MY_PROJECTS := NONE

ifeq ($(MODE),full)
  MY_PROJECTS := API CLI GUI JNI
  export ENABLE_JNI := true
endif
ifeq ($(MODE),no-gui)
  MY_PROJECTS := API CLI JNI
  export ENABLE_JNI := true
endif
ifeq ($(MODE),minimal)
  MY_PROJECTS := API CLI
  export ENABLE_JNI := false
endif

ifeq ($(MY_PROJECTS),NONE)
  $(error Invalid MODE value: $(MODE))
endif

##############################################################################
# Rules
##############################################################################

BUILD_PROJECTS = $(addprefix DynamicAudioNormalizer,$(MY_PROJECTS))
CLEAN_PROJECTS = $(addprefix CleanUp,$(BUILD_PROJECTS))

.PHONY: all clean $(BUILD_PROJECTS) $(CLEAN_PROJECTS) DeployBinaries CopyAllBinaries CreateDocuments CreateTagFile

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
	make -C ./$(patsubst CleanUp%,%,$@) clean

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

DeployBinaries: CopyAllBinaries CreateDocuments CreateTagFile
	@$(ECHO) "\n\e[1;34m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;34mDeploy\e[0m"
	@$(ECHO) "\e[1;34m-----------------------------------------------------------------------------\n\e[0m"
	rm -f $(OUTPUT_FILE)
	pushd $(TARGET_PATH) ; tar -vcjf $(OUTPUT_FILE) * ; popd

CreateTagFile:
	@$(ECHO) "\n\e[1;34m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;34mBuild Tag\e[0m"
	@$(ECHO) "\e[1;34m-----------------------------------------------------------------------------\n\e[0m"
	echo "Dynamic Audio Normalizer" > $(BUILD_TAG)
	echo "Copyright (C) 2015 LoRd_MuldeR <MuldeR2@GMX.de>" >> $(BUILD_TAG)
	echo "" >> $(BUILD_TAG)
	echo "Version $$(printf %d.%02d-%d $(VER_MAJOR) $(VER_MINOR) $(VER_PATCH)). Built on $(BUILD_DATE), at $(BUILD_TIME)" >> $(BUILD_TAG)
	echo "" >> $(BUILD_TAG)
	g++ --version | head -n1 | sed 's/^/Compiler version:   /' >> $(BUILD_TAG)
	uname -srmo | sed 's/^/Build platform:     /' >> $(BUILD_TAG)
	(lsb_release -s -d || echo "Unknown") | sed 's/\"//g' | sed 's/^/System description: /' >> $(BUILD_TAG)
	echo "" >> $(BUILD_TAG)
	echo "This library is free software; you can redistribute it and/or" >> $(BUILD_TAG)
	echo "modify it under the terms of the GNU Lesser General Public" >> $(BUILD_TAG)
	echo "License as published by the Free Software Foundation; either" >> $(BUILD_TAG)
	echo "version 2.1 of the License, or (at your option) any later version." >> $(BUILD_TAG)
	echo "" >> $(BUILD_TAG)
	echo "This library is distributed in the hope that it will be useful," >> $(BUILD_TAG)
	echo "but WITHOUT ANY WARRANTY; without even the implied warranty of" >> $(BUILD_TAG)
	echo "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU" >> $(BUILD_TAG)
	echo "Lesser General Public License for more details." >> $(BUILD_TAG)
	mkdir -p $(TARGET_PATH)
	mv -f $(BUILD_TAG) $(TARGET_PATH)/BUILD_TAG

CopyAllBinaries:
	@$(ECHO) "\n\e[1;34m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;34mCopy Binaries\e[0m"
	@$(ECHO) "\e[1;34m-----------------------------------------------------------------------------\n\e[0m"
	rm -rf $(TARGET_PATH) 
	mkdir -p $(TARGET_PATH)/include
	cp $(PROGRAM_NAME)/bin/$(PROGRAM_NAME).* $(TARGET_PATH)
	cp $(LIBRARY_NAME)/lib/lib$(LIBRARY_NAME)-$(API_VERSION).* $(TARGET_PATH)
	cp $(LOGVIEW_NAME)/bin/$(LOGVIEW_NAME).* $(TARGET_PATH) || $(ECHO) "\e[1;33mWARNING: File \"$(LOGVIEW_NAME)\" not found!\e[0m"
	cp $(JNIWRAP_NAME)/out/$(JNIWRAP_NAME).jar $(TARGET_PATH) || $(ECHO) "\e[1;33mWARNING: File \"$(JNIWRAP_NAME).jar\" not found!\e[0m"
	cp $(LIBRARY_NAME)/include/*.h $(TARGET_PATH)/include
	cp $(PASWRAP_NAME)/include/*.pas $(TARGET_PATH)/include
	cp ./LICENSE-*.html $(TARGET_PATH)

CreateDocuments:
	@$(ECHO) "\n\e[1;34m-----------------------------------------------------------------------------\e[0m"
	@$(ECHO) "\e[1;34mCreate Documents\e[0m"
	@$(ECHO) "\e[1;34m-----------------------------------------------------------------------------\n\e[0m"
	mkdir -p $(TARGET_PATH)/img/dyauno
	$(PANDOC) --from $(PANDOC_FLAGS) --to html5 --toc -N --standalone -H ./img/dyauno/Style.inc ./README.md --output $(TARGET_PATH)/README.html || cp ./README.md $(TARGET_PATH)/README.md
	cp ./img/dyauno/*.png $(TARGET_PATH)/img/dyauno

