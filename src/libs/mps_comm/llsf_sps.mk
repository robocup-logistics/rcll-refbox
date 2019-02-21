#*****************************************************************************
#           Makefile Build System for Fawkes: llsf_sps Library
#                            -------------------
#   Created on Thu Feb 14 09:49:27 2013
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

ifneq ($(PKGCONFIG),)
  HAVE_LIBMODBUS = $(if $(shell $(PKGCONFIG) --exists 'libmodbus'; echo $${?/1/}),1,0)
endif

ifeq ($(HAVE_LIBMODBUS),1)
  CFLAGS_LIBMODBUS  = $(shell $(PKGCONFIG) --cflags 'libmodbus')
  LDFLAGS_LIBMODBUS = $(shell $(PKGCONFIG) --libs 'libmodbus')
endif
