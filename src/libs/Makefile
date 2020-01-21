#*****************************************************************************
#                      Makefile Build System for Fawkes
#                            -------------------
#   Created on Fri Jan 18 00:17:05 2013
#   Copyright (C) 2006-2011 by Tim Niemueller, Carologistics RoboCup Team
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

SUBDIRS = core utils config logging netcomm protobuf_comm protobuf_clips \
	  llsf_sps mongodb_log mps_placing_clips webview rest_api

# Explicit dependencies, this is needed to have make bail out if there is any
# error. This is also necessary for working parallel build (i.e. for dual core)
netcomm config mongodb_log: core utils
utils llsf_sps mps_placing_clips: core
logging: core protobuf_comm
protobuf_clips: protobuf_comm
mongodb_log: logging

include $(BUILDSYSDIR)/base.mk
