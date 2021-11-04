#*****************************************************************************
#           Makefile Build System for Fawkes: mps_placing Library
#                            -------------------
#   Created on Tue Apr 16 13:38:13 2013
#   Copyright (C) 2013 by Tim Niemueller, AllemaniACs RoboCup Team
#   							2017		Tobias Neumann
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
include $(BUILDSYSDIR)/boost.mk
include $(BUILDSYSDIR)/clips.mk

CFLAGS += $(CFLAGS_CPP17)

REQ_BOOST_LIBS = graph
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

LIBS_libllsf_mps_placing_clips = stdc++ m llsfrbcore
OBJS_libllsf_mps_placing_clips = $(patsubst %.cpp,%.o,$(patsubst qa/%,,$(subst $(SRCDIR)/,,$(realpath $(wildcard $(SRCDIR)/*.cpp)))))
HDRS_libllsf_mps_placing_clips = $(subst $(SRCDIR)/,,$(wildcard $(SRCDIR)/*.h))

OBJS_all = $(OBJS_libllsf_mps_placing_clips)

HAVE_GECODE = 0
ifneq ($(wildcard /usr/include/gecode/kernel.hh),)
  ifneq ($(wildcard /usr/lib64/libgecodekernel.so),1)
    HAVE_GECODE = 1
  endif
endif

ifeq ($(HAVE_CLIPS)$(HAVE_GECODE)$(HAVE_BOOST_LIBS),111)
  CFLAGS  += $(CFLAGS_CLIPS) -I/usr/include/gecode/ \
       $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS += $(LDFLAGS_CLIPS) -lgecodeint -lgecodekernel -lgecodesupport \
              -lgecodesearch -lgecodedriver -lgecodeminimodel \
       $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  LIBS_all  = $(LIBDIR)/libllsf_mps_placing_clips.so
else
  ifneq ($(HAVE_CLIPS),1)
    WARN_TARGETS = warning_clips
  endif
  ifneq ($(HAVE_GECODE),1)
    WARN_TARGETS = warning_gecode
  endif
  ifneq ($(HAVE_BOOST_LIBS),1)
    WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
  endif
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
warning_clips:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build mps_placing_clips library$(TNORMAL) (clipsmm not found)"
warning_gecode:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build mps_placing_clips library$(TNORMAL) (gecode not found)"
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build protobuf_comm library$(TNORMAL) (Boost library $* not found)"
endif

include $(BUILDSYSDIR)/base.mk

