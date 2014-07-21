#*****************************************************************************
#            Makefile Build System for Fawkes: MongoDB Plugin
#                            -------------------
#   Created on Sun Dec 05 23:03:18 2010 (Steelers vs. Baltimore)
#   Copyright (C) 2006-2010 by Tim Niemueller, AllemaniACs RoboCup Team
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

include $(BUILDSYSDIR)/boost.mk

ifneq ($(PKGCONFIG),)
  HAVE_LIBSSL := $(if $(shell $(PKGCONFIG) --exists 'openssl'; echo $${?/1/}),1,0)
endif
ifeq ($(HAVE_LIBSSL),1)
  CFLAGS_LIBSSL  += -DHAVE_LIBSSL $(shell $(PKGCONFIG) --cflags 'openssl')
  LDFLAGS_LIBSSL += $(shell $(PKGCONFIG) --libs 'openssl')
endif

ifneq ($(wildcard /usr/include/mongo/client/dbclient.h /usr/local/include/mongo/client/dbclient.h),)
  ifeq ($(call boost-have-libs,thread system filesystem)$(HAVE_LIBSSL),11)
    HAVE_MONGODB = 1
    CFLAGS_MONGODB  = -DHAVE_MONGODB $(CFLAGS_LIBSSL)
    LDFLAGS_MONGODB = -lm -lpthread \
		      $(call boost-libs-ldflags,thread system filesystem) \
		      $(LDFLAGS_LIBSSL)
  endif
endif

