#*****************************************************************************
#                Makefile Build System for Fawkes: protobuf bits
#                            -------------------
#   Created on Wed Jan 16 17:11:07 2013
#   Copyright (C) 2012 by Tim Niemueller, AllemaniACs RoboCup Team
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
$(error config.mk must be included before protobuf.mk)
endif

ifndef __buildsys_protobuf_mk_
__buildsys_protobuf_mk_ := 1


ifneq ($(PKGCONFIG),)
  HAVE_PROTOBUF = $(if $(shell $(PKGCONFIG) --exists 'protobuf'; echo $${?/1/}),1,0)
endif

ifeq ($(HAVE_PROTOBUF),1)
  CFLAGS_PROTOBUF  = -DHAVE_PROTOBUF $(shell $(PKGCONFIG) --cflags 'protobuf')
  LDFLAGS_PROTOBUF = $(shell $(PKGCONFIG) --libs 'protobuf')

  PROTOBUF_PROTOC = protoc
  PROTOBUF_LIBDIR = $(LIBDIR)/protobuf

ifneq ($(PROTOBUF_all),)
  $(foreach P,$(PROTOBUF_all),							\
	$(eval LDFLAGS_protobuf_lib$P     = $(LDFLAGS_PROTOBUF))		\
	$(eval OBJS_protobuf_lib$P        = $P.pb.o)				\
	$(eval HDRS_interfaces_lib$P      = $P.pb.h)				\
	$(eval INST_LIB_SUBDIR_protobuf_lib$P   = $(FFLIBSUBDIR))		\
	$(eval INST_HDRS_SUBDIR_protobuf_lib$P  = protobuf)			\
	$(eval OBJS_all                  += $$(OBJS_protobuf_lib$P))		\
	$(eval PROTOBUF_SRCS             += $(SRCDIR)/$P.pb.cpp)		\
	$(eval PROTOBUF_HDRS             += $(SRCDIR)/$P.pb.h)			\
	$(eval PROTOBUF_LIBS             += $(PROTOBUF_LIBDIR)/lib$P.$(SOEXT))	\
	$(eval PROTOBUF_TOUCH            += $(SRCDIR)/$(OBJDIR)/$P.touch)	\
	$(eval PROTOBUF_OBJS             += $P.pb.o)				\
	$(eval LIBS_all                  += $$(PROTOBUF_LIBDIR)/lib$P.$(SOEXT))	\
	$(eval CLEAN_FILES               += $P.pb.h $P.pb.cpp)			\
  )
endif

ifeq ($(OBJSSUBMAKE),1)

$(PROTOBUF_SRCS): $(SRCDIR)/%.pb.cpp: $(SRCDIR)/$(OBJDIR)/%.touch
$(PROTOBUF_HDRS): $(SRCDIR)/%.pb.h: $(SRCDIR)/$(OBJDIR)/%.touch


$(PROTOBUF_TOUCH): $(SRCDIR)/$(OBJDIR)/%.touch: $(SRCDIR)/%.proto
	$(SILENTSYMB) echo "$(INDENT_PRINT)--> Generating $* (Protobuf Message)"
	$(SILENT)$(PROTOBUF_PROTOC) --cpp_out $(SRCDIR) --proto_path $(SRCDIR) $<
	$(SILENT) mv $(SRCDIR)/$*.pb.cc $(SRCDIR)/$*.pb.cpp
	$(SILENT) mkdir -p $(@D)
	$(SILENT) touch $@

.SECONDARY: $(PROTOBUF_SRCS) $(PROTOBUF_HDRS)

endif # OBJSSUBMAKE != 1

ifneq ($(PLUGINS_all),)
$(PLUGINS_all:%.$(SOEXT)=%.$(SOEXT)): | $(PROTOBUF_LIBS:%.$(SOEXT)=%.$(SOEXT))
endif
ifneq ($(BINS_all),)
$(BINS_all): | $(PROTOBUF_LIBS)
endif

else
  ifneq ($(PROTOBUF_all),)
    WARN_TARGETS += warning_protobuf
  endif

  ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS)
.PHONY: warning_protobuf
warning_nolib:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build protobuf messages$(TNORMAL) (protobuf not found)"
  endif

endif # HAVE_PROTOBUF


endif # __buildsys_protobuf_mk_
