#!gmake
#
# Copyright (c) 2024 Ken McDonell.  All Rights Reserved.
#

TOPDIR = ../..
include $(TOPDIR)/src/include/builddefs

TESTDIR = $(PCP_VAR_DIR)/testsuite/pdudata

FILES =  $(shell echo [a-z]*)

default default_pcp setup: $(FILES)

install install_pcp:
	$(INSTALL) -m 755 -d $(TESTDIR)
	$(INSTALL) -m 644 -f $(FILES) $(TESTDIR)

include $(BUILDRULES)
