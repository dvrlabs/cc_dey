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
#
# Final directory tree structure
# ------------------------------
# ./cc_dey
#    |-src
#       |-ccapi
#       |  |-include
#       |  |  |-ccapi
#       |  |  |-ccimp
#       |  |  |-custom
#       |  |-source
#       |     |-ccfsm
#       |        |-include
#       |        |  |-api
#       |        |  |-custom
#       |        |-source
#       |-ccimp
#       |-custom
#

hide = @

TARBALL = cc_dey.tar.gz

OUTPUT_DIR = cc_dey

LOCAL_SRC_DIR = src
LOCAL_CCAPI_DIR = $(LOCAL_SRC_DIR)/cc_api
LOCAL_CCFSM_DIR = $(LOCAL_CCAPI_DIR)/source/cc_ansic

MAKEFILE = Makefile

all: $(TARBALL)

$(TARBALL):
	@# Remove output directory in case it already exsists.
	$(hide) rm -rf $(OUTPUT_DIR)

	@echo "Creating output directories..."
	$(hide) mkdir -p $(OUTPUT_DIR)/src/ccapi/include/ccapi
	$(hide) mkdir -p $(OUTPUT_DIR)/src/ccapi/include/ccimp
	$(hide) mkdir -p $(OUTPUT_DIR)/src/ccapi/include/custom
	$(hide) mkdir -p $(OUTPUT_DIR)/src/ccapi/source/ccfsm/include/api
	$(hide) mkdir -p $(OUTPUT_DIR)/src/ccapi/source/ccfsm/include/custom
	$(hide) mkdir -p $(OUTPUT_DIR)/src/ccapi/source/ccfsm/source
	$(hide) mkdir -p $(OUTPUT_DIR)/src/ccapi/source/custom_include
	$(hide) mkdir -p $(OUTPUT_DIR)/src/ccimp
	$(hide) mkdir -p $(OUTPUT_DIR)/src/custom

	@echo "Copying files..."
	$(hide) cp $(LOCAL_CCFSM_DIR)/public/include/*.h $(OUTPUT_DIR)/src/ccapi/source/ccfsm/include
	$(hide) cp $(LOCAL_CCFSM_DIR)/public/include/api/*.h $(OUTPUT_DIR)/src/ccapi/source/ccfsm/include/api
	$(hide) cp $(LOCAL_CCFSM_DIR)/public/include/custom/*.h $(OUTPUT_DIR)/src/ccapi/source/ccfsm/include/custom
	$(hide) cp $(LOCAL_CCFSM_DIR)/private/*.[ch] $(OUTPUT_DIR)/src/ccapi/source/ccfsm/source
	$(hide) cp $(LOCAL_CCAPI_DIR)/source/*.[ch] $(OUTPUT_DIR)/src/ccapi/source
	$(hide) cp $(LOCAL_CCAPI_DIR)/include/ccapi/*.h $(OUTPUT_DIR)/src/ccapi/include/ccapi
	$(hide) cp $(LOCAL_CCAPI_DIR)/include/ccimp/*.h $(OUTPUT_DIR)/src/ccapi/include/ccimp
	$(hide) cp $(LOCAL_CCAPI_DIR)/include/custom/*.h $(OUTPUT_DIR)/src/ccapi/include/custom
	$(hide) cp $(LOCAL_CCAPI_DIR)/source/cc_ansic_custom_include/*.h $(OUTPUT_DIR)/src/ccapi/source/custom_include
	$(hide) cp $(LOCAL_SRC_DIR)/*.[ch] $(OUTPUT_DIR)/src
	$(hide) cp $(LOCAL_SRC_DIR)/custom/*.[ch] $(OUTPUT_DIR)/src/custom
	$(hide) cp $(LOCAL_SRC_DIR)/ccimp/*.[ch] $(OUTPUT_DIR)/src/ccimp
	$(hide) cp $(MAKEFILE) $(OUTPUT_DIR)/$(MAKEFILE)

	@echo "Updating Makefile..."
	$(hide) sed -i  -e '/^CCAPI_DIR =/{s,cc_api,ccapi,g}' \
		-e '/^CCFSM_DIR =/{s,cc_ansic,ccfsm,g}' \
		-e '/^CUSTOM_CCFSM_PUBLIC_HEADER_DIR =/{s,cc_ansic_custom_include,custom_include,g}' \
		-e '/^CCFSM_PUBLIC_HEADER_DIR =/{s,public/include,include,g}' \
		-e '/^CCFSM_PRIVATE_DIR =/{s,private,source,g}' \
		$(OUTPUT_DIR)/$(MAKEFILE)

	@echo "Creating tarball..."
	$(hide) tar c --numeric-owner --owner=root --group=root $(OUTPUT_DIR) | gzip -n > $@
	$(hide) rm -rf $(OUTPUT_DIR)

	@echo "Done!"

.PHONY: clean	
clean:
	-rm -rf $(TARBALL) $(OUTPUT_DIR)
