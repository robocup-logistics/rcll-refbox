#*****************************************************************************
#           Makefile Build System for Fawkes: mongodb_log Library
#                            -------------------
#   Created on Fri Jun 07 16:26:05 2013
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
include $(SRCDIR)/mongodb.mk

REQ_BOOST_LIBS = thread asio system signals2
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

CFLAGS += $(CFLAGS_CPP11)

LIBS_libllsf_mongodb_log = stdc++ m llsfrbcore llsfrblogging
OBJS_libllsf_mongodb_log = $(patsubst %.cpp,%.o,$(patsubst qa/%,,$(subst $(SRCDIR)/,,$(realpath $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*/*.cpp $(SRCDIR)/*/*/*.cpp)))))
HDRS_libllsf_mongodb_log = $(subst $(SRCDIR)/,,$(wildcard $(SRCDIR)/*.h $(SRCDIR)/*/*.h $(SRCDIR)/*/*/*.h))

OBJS_all = $(OBJS_libllsf_mongodb_log)

ifeq ($(HAVE_PROTOBUF)$(HAVE_MONGODB)$(HAVE_BOOST_LIBS),111)
  CFLAGS  += $(CFLAGS_PROTOBUF)  $(CFLAGS_MONGODB)  $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS += $(LDFLAGS_PROTOBUF) -lmongoclient $(LDFLAGS_MONGODB) $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  LIBS_all  = $(LIBDIR)/libllsf_mongodb_log.$(SOEXT)
else
  ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
  endif
  ifneq ($(HAVE_MONGODB),1)
    WARN_TARGETS += warning_mongodb
  endif
  ifneq ($(HAVE_BOOST_LIBS),1)
    WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
  endif
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: $(WARN_TARGETS)
warning_protobuf:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build mongodb_log library$(TNORMAL) (protobuf not found)"

warning_mongodb:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build mongodb_log library$(TNORMAL) (MongoDB not found)"

$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build mongodb_log library$(TNORMAL) (Boost library $* not found)"

endif

include $(BUILDSYSDIR)/base.mk

