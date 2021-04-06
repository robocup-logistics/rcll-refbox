#! /bin/bash
#
# restore_reports.bash restore a mongo dump from an archive
#
# Copyright (C) 2021 Tarik Viehmann <viehmann@kbsg.rwth-aachen.de>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  Read the full text in the LICENSE.GPL file in the doc directory.
echo "------------------------------------------------------------------------"
echo "Usage: ./restore_reports.bash <dump archive>"
echo "Loads dumped game reports from an archive"
echo "------------------------------------------------------------------------"
echo ""

CONFIG=${LLSF_REFBOX_DIR}/cfg/config.yaml
if [ -z "$1" ]
  then
    echo "expected 1 argument"
		exit
fi

mongorestore --gzip --archive=$1
