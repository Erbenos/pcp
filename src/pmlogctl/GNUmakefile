#
# Copyright (c) 2020 Ken McDonell.  All Rights Reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

TOPDIR = ../..
include $(TOPDIR)/src/include/builddefs

default :

include $(BUILDRULES)

install : default
	$(INSTALL) -m 755 pmlogctl.sh $(PCP_BIN_DIR)/pmlogctl$(SHELLSUFFIX)
	$(INSTALL) -m 755 -d $(PCP_ETC_DIR)/pcp/pmlogger/class.d
	$(INSTALL) -S $(PCP_BIN_DIR)/pmlogctl$(SHELLSUFFIX) $(PCP_BIN_DIR)/pmiectl$(SHELLSUFFIX)
	$(INSTALL) -m 755 -d $(PCP_ETC_DIR)/pcp/pmie/class.d

default_pcp : default

install_pcp : install