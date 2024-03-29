#*****************************************************************************
#               Makefile Build System for Fawkes : Curses Shell
#                            -------------------
#   Created on Fri Feb 15 10:03:02 2013
#   Copyright (C) 2013 by Tim Niemueller, Carologistics RoboCup Team
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

BASEDIR = ../..

include $(BASEDIR)/etc/buildsys/config.mk
include $(BUILDSYSDIR)/protobuf.mk
include $(BUILDSYSDIR)/boost.mk

CFLAGS += $(CFLAGS_CPP11)

REQ_BOOST_LIBS = thread asio system signals2 format date_time
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

REQ_NCURSES_LIBS = ncursesw formw panelw menuw

ifneq ($(PKGCONFIG),)
  HAVE_NCURSES = $(if $(shell $(PKGCONFIG) --exists '$(REQ_NCURSES_LIBS)'; echo $${?/1/}),1,0)
endif

ifeq ($(HAVE_NCURSES),1)
  CFLAGS_NCURSES  = $(shell $(PKGCONFIG) --cflags '$(REQ_NCURSES_LIBS)')
  LDFLAGS_NCURSES = $(shell $(PKGCONFIG) --libs '$(REQ_NCURSES_LIBS)') -lncurses++w
endif

LIBS_llsf_refbox_shell = m stdc++ llsfrbcore llsfrbconfig \
			 rcll_msgs llsf_log_msgs
OBJS_llsf_refbox_shell = main.o shell.o machine.o robot.o order.o menus.o colors.o

ifeq ($(HAVE_PROTOBUF)$(HAVE_PROTOBUF_COMM)$(HAVE_BOOST_LIBS)$(HAVE_NCURSES),111)
  OBJS_all =	$(OBJS_llsf_refbox_shell)
  BINS_all =	$(BINDIR)/llsf-refbox-shell

  CFLAGS  += $(CFLAGS_PROTOBUF) $(CFLAGS_PROTOBUF_COMM) $(CFLAGS_NCURSES) \
	     $(call boost-libs-cflags,$(REQ_BOOST_LIBS)) \
	     -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
  LDFLAGS += $(LDFLAGS_PROTOBUF) $(CFLAGS_PROTOVUF_COMM) $(LDFLAGS_NCURSES) \
	     $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))
  #MANPAGES_all =  $(MANDIR)/man1/refbox-llsf.1
else
  ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
  endif
  ifneq ($(HAVE_PROTOBUF_COMM),1)
    WARN_TARGETS += warning_protobuf_comm
  endif
  ifneq ($(HAVE_BOOST_LIBS),1)
    WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
  endif
  ifneq ($(HAVE_NCURSES),1)
    WARN_TARGETS += warning_ncurses
  endif
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: $(WARN_TARGETS) $(WARN_TARGETS_BOOST) warning_ncurses
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF RefBox shell$(TNORMAL) (Boost library $* not found)"
warning_ncurses:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF RefBox shell$(TNORMAL) (ncurses++ not found)"
endif

include $(BUILDSYSDIR)/base.mk
