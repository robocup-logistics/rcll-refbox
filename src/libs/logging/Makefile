#*****************************************************************************
#           Makefile Build System for LLSF RefBox: Logging Utils
#                            -------------------
#   Created on Sat May 28 00:16:57 2011
#   Copyright (C) 2006-2011 by Tim Niemueller, AllemaniACs RoboCup Team
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


REQ_BOOST_LIBS = system
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

PRESUBDIRS = llsf_log_msgs
CFLAGS += $(CFLAGS_CPP11)

ifeq ($(HAVE_PROTOBUF)$(HAVE_BOOST_LIBS),11)

  CFLAGS  += $(CFLAGS_PROTOBUF) \
	     $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS += $(LDFLAGS_PROTOBUF) \
	     $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  LIBS_libllsfrblogging = stdc++ llsfrbcore llsf_protobuf_comm llsf_log_msgs
  OBJS_libllsfrblogging = $(patsubst %.cpp,%.o,$(patsubst qa/%,,$(subst $(SRCDIR)/,,$(realpath $(wildcard $(SRCDIR)/*.cpp)))))
  HDRS_libllsfrblogging = $(subst $(SRCDIR)/,,$(wildcard $(SRCDIR)/*.h))

  LIBS_all  = $(LIBDIR)/libllsfrblogging.so
  OBJS_all = $(OBJS_libllsfrblogging)
else
  ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
  endif
  ifneq ($(HAVE_BOOST_LIBS),1)
    WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
  endif
endif

network.o: | $(PRESUBDIRS)


ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build logging library$(TNORMAL) (Boost library $* not found)"

endif

include $(BUILDSYSDIR)/base.mk

