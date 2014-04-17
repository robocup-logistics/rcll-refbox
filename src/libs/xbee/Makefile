#*****************************************************************************
#           Makefile Build System for Fawkes: XBee Library
#                            -------------------
#   Created on Tue Apr 15 21:44:39 2014
#   Copyright (C) 2013 by Tim Niemueller, AllemaniACs RoboCup Team
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

BASEDIR = ../../..
include $(BASEDIR)/etc/buildsys/config.mk
include $(BUILDSYSDIR)/protobuf.mk
include $(BUILDSYSDIR)/boost.mk

LIBS_libllsfxbee = stdc++ m
OBJS_libllsfxbee = xbee.o
HDRS_libllsfxbee = xbee.h

OBJS_all = $(OBJS_libllsfxbee)

ifeq ($(CC),gcc)
  ifneq ($(call gcc_atleast_version,4,6),1)
    GCC_TOO_OLD=1
  endif
endif

REQ_BOOST_LIBS = thread asio system signals2
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

ifneq ($(GCC_TOO_OLD),1)
  ifeq ($(HAVE_CPP11)$(HAVE_BOOST_LIBS),11)
    CFLAGS  += $(CFLAGS_CPP11) \
	     $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
    LDFLAGS += $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

    LIBS_all  = $(LIBDIR)/libllsfxbee.so
  else
    ifneq ($(HAVE_CPP11),1)
      WARN_TARGETS += warning_cpp11
    endif
    ifneq ($(HAVE_BOOST_LIBS),1)
      WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
    endif
  endif
else
      WARN_TARGETS += warning_old_gcc
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
warning_cpp11:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF XBee library$(TNORMAL) (compiler has no C++11 support)"
warning_old_gcc:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF XBee library$(TNORMAL) (GCC too old, have $(GCC_VERSION), required 4.6)"
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF XBee library$(TNORMAL) (Boost library $* not found)"
endif

include $(BUILDSYSDIR)/base.mk

