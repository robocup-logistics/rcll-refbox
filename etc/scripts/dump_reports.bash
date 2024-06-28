#! /bin/bash
# Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

#
# dump_reports.bash dump the game report collection to an archive
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
echo "Usage: ./dump_reports.bash <name>"
echo "Dumps the stored game reports to an archive >name>.gz"
echo "------------------------------------------------------------------------"
echo ""

CONFIG=${LLSF_REFBOX_DIR}/cfg/config.yaml
if [ -z "$1" ]
  then
    echo "expected 1 argument"
		exit
fi

mongodump -d rcll -c game_report --gzip --archive=$1.gz
