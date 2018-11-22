#*****************************************************************************
#                      Makefile Build System for Fawkes
#                            -------------------
#   Created on Sun Sep 03 14:14:14 2006
#   Copyright (C) 2006-2007 by Tim Niemueller, AllemaniACs RoboCup Team
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

ifndef __buildsys_config_mk_
$(error config.mk must be included before base.mk)
endif

.DEFAULT:

ifneq ($(OBJDIR),$(notdir $(CURDIR)))
  ifneq (clean,$(MAKECMDGOALS))
    include $(BUILDSYSDIR)/objsdir.mk
  else
    include $(BUILDSYSDIR)/rules.mk
  endif
else
  include $(BUILDSYSDIR)/rules.mk
endif

ifeq ($(FAIL_ON_WARNING),1)
  ifneq ($(WARN_TARGETS),)
all: $(WARN_TARGETS) fail_on_warning
  endif
endif

.PHONY: fail_on_warning
fail_on_warning:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)A warning occurred and all warnings are considered to be errors!$(TNORMAL) Build with FAIL_ON_WARNING=0 to disable."
	$(SILENT)exit 1
