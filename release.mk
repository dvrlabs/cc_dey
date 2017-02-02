# ***************************************************************************
# Copyright (c) 2017 Digi International Inc.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#
# Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
#
# ***************************************************************************

hide = @

NAME = cloudconnector
VERSION = 1.0

OUTPUT_DIR = $(NAME)-$(VERSION)
TARBALL = $(NAME)-$(VERSION).tar.gz

APP_DIR = app
LIB_DIR = library
LOCAL_CC_API_DIR = $(LIB_DIR)/src/cc_api
LOCAL_CC_ANSIC_DIR = $(LOCAL_CC_API_DIR)/source/cc_ansic
OUTPUT_CC_API_DIR = $(OUTPUT_DIR)/$(LIB_DIR)/src/cc_api
OUTPUT_CC_ANSIC_DIR = $(OUTPUT_CC_API_DIR)/source/cc_ansic

MAKEFILE = Makefile

CONFIG_GENERATOR_BUILD = $(LOCAL_CC_ANSIC_DIR)/tools/config/build.xml
CONFIG_GENERATOR_BIN = $(LOCAL_CC_ANSIC_DIR)/tools/config/dist/ConfigGenerator.jar

all: $(TARBALL)

$(CONFIG_GENERATOR_BIN):
	@echo "Building ConfigGenerator..."
	$(hide) ant -f $(CONFIG_GENERATOR_BUILD)

$(TARBALL): $(CONFIG_GENERATOR_BIN)
	@# Remove output directory in case it already exsists.
	$(hide) rm -rf $(OUTPUT_DIR)

	@echo "Creating output directories..."
	$(hide) mkdir -p $(OUTPUT_CC_API_DIR)/include/ccapi
	$(hide) mkdir -p $(OUTPUT_CC_API_DIR)/include/ccimp
	$(hide) mkdir -p $(OUTPUT_CC_API_DIR)/include/custom
	$(hide) mkdir -p $(OUTPUT_CC_API_DIR)/source/cc_ansic_custom_include
	$(hide) mkdir -p $(OUTPUT_CC_ANSIC_DIR)/private
	$(hide) mkdir -p $(OUTPUT_CC_ANSIC_DIR)/public/certificates
	$(hide) mkdir -p $(OUTPUT_CC_ANSIC_DIR)/public/include/api
	$(hide) mkdir -p $(OUTPUT_CC_ANSIC_DIR)/public/include/custom
	$(hide) mkdir -p $(OUTPUT_DIR)/$(LIB_DIR)/src/ccimp
	$(hide) mkdir -p $(OUTPUT_DIR)/$(LIB_DIR)/src/custom
	$(hide) mkdir -p $(OUTPUT_DIR)/$(LIB_DIR)/src/miniunz
	$(hide) mkdir -p $(OUTPUT_DIR)/$(APP_DIR)/cfg_files
	$(hide) mkdir -p $(OUTPUT_DIR)/$(LIB_DIR)/rci
	$(hide) mkdir -p $(OUTPUT_DIR)/$(APP_DIR)/src

	@echo "Copying files..."
	$(hide) cp $(LOCAL_CC_ANSIC_DIR)/public/include/*.h $(OUTPUT_CC_ANSIC_DIR)/public/include
	$(hide) cp $(LOCAL_CC_ANSIC_DIR)/public/include/api/*.h $(OUTPUT_CC_ANSIC_DIR)/public/include/api
	$(hide) cp $(LOCAL_CC_ANSIC_DIR)/public/include/custom/*.h $(OUTPUT_CC_ANSIC_DIR)/public/include/custom
	$(hide) cp $(LOCAL_CC_ANSIC_DIR)/private/*.[ch] $(OUTPUT_CC_ANSIC_DIR)/private
	$(hide) cp $(LOCAL_CC_ANSIC_DIR)/public/certificates/*.crt $(OUTPUT_CC_ANSIC_DIR)/public/certificates
	$(hide) cp $(LOCAL_CC_API_DIR)/source/*.[ch] $(OUTPUT_CC_API_DIR)/source
	$(hide) cp $(LOCAL_CC_API_DIR)/include/ccapi/*.h $(OUTPUT_CC_API_DIR)/include/ccapi
	$(hide) cp $(LOCAL_CC_API_DIR)/include/ccimp/*.h $(OUTPUT_CC_API_DIR)/include/ccimp
	$(hide) cp $(LOCAL_CC_API_DIR)/include/custom/*.h $(OUTPUT_CC_API_DIR)/include/custom
	$(hide) cp $(LOCAL_CC_API_DIR)/source/cc_ansic_custom_include/*.h $(OUTPUT_CC_API_DIR)/source/cc_ansic_custom_include
	$(hide) cp $(LIB_DIR)/src/*.[ch] $(OUTPUT_DIR)/$(LIB_DIR)/src
	$(hide) cp $(LIB_DIR)/src/custom/*.[ch] $(OUTPUT_DIR)/$(LIB_DIR)/src/custom
	$(hide) cp $(LIB_DIR)/src/ccimp/*.[ch] $(OUTPUT_DIR)/$(LIB_DIR)/src/ccimp
	$(hide) cp $(LIB_DIR)/src/miniunz/*.[ch] $(OUTPUT_DIR)/$(LIB_DIR)/src/miniunz
	$(hide) cp $(LIB_DIR)/src/config.rci $(OUTPUT_DIR)/$(LIB_DIR)/rci
	$(hide) cp $(LIB_DIR)/cloudconnector.pc $(OUTPUT_DIR)/$(LIB_DIR)/cloudconnector.pc
	$(hide) cp $(LIB_DIR)/$(MAKEFILE) $(OUTPUT_DIR)/$(LIB_DIR)/$(MAKEFILE)
	$(hide) cp $(APP_DIR)/src/*.[ch] $(OUTPUT_DIR)/$(APP_DIR)/src
	$(hide) cp $(APP_DIR)/cfg_files/* $(OUTPUT_DIR)/$(APP_DIR)/cfg_files
	$(hide) cp $(APP_DIR)/$(MAKEFILE) $(OUTPUT_DIR)/$(APP_DIR)/$(MAKEFILE)
	$(hide) cp $(CONFIG_GENERATOR_BIN) $(OUTPUT_DIR)/$(LIB_DIR)/rci
	$(hide) cp $(MAKEFILE) $(OUTPUT_DIR)/$(MAKEFILE)
	$(hide) cp README.md $(OUTPUT_DIR)

	@echo "Creating tarball..."
	$(hide) tar c --numeric-owner --owner=root --group=root $(OUTPUT_DIR) | gzip -n > $@
	$(hide) rm -rf $(OUTPUT_DIR)

	@echo "Done!"

.PHONY: clean	
clean:
	-rm -rf $(TARBALL) $(OUTPUT_DIR) $(CONFIG_GENERATOR_BIN)
