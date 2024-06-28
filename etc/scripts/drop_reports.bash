#! /bin/bash
# Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

#
# drop_reports.bash dropsa all stored game reports, use with caution
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
echo "Usage: ./drop_reports.bash"
echo "Drops all stored game reports"
echo "------------------------------------------------------------------------"
echo ""

read -p "Are you sure? [Yy]" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
	mongosh rcll --eval 'db.game_report.drop()'
else
	echo "abort"
fi
