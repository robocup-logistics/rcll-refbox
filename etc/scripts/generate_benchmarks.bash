#! /bin/bash
#
# generate_benchmarks.bash Generate multiple game reports to get benchmark sets
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
                                                                           #
echo "------------------------------------------------------------------------"
echo "Usage: ./generate_benchmarks.bash <report name> <number of reports>"
echo "Generates entries <report name>_<i> where i in [1...<number of reports>]"
echo "------------------------------------------------------------------------"
echo " "

CONFIG_MGDB=${LLSF_REFBOX_DIR}/cfg/mongodb/default_mongodb.yaml
CONFIG_SIM=${LLSF_REFBOX_DIR}/cfg/simulation/default_simulation.yaml
CONFIG_GME=${LLSF_REFBOX_DIR}/cfg/game/default_game.yaml
if [ -z "$1" ] || [ -z "$2" ]
	then
		echo "expected 2 arguments"
		exit
fi

mongo --eval "print(\"Test for connection\")" &>/dev/null
if [ $? -ne 0 ]
	then
		echo "Mongo instance not running, abort."
		exit
fi

NAME_PREFIX=$1
NUM_BENCHMARKS=$2

echo "Creating Backup of original config"
cp ${CONFIG_MGDB} ${CONFIG_MGDB}.bak
cp ${CONFIG_SIM} ${CONFIG_SIM}.bak
cp ${CONFIG_GME} ${CONFIG_GME}.bak

echo "Setup RefBox config"
sed -E -n -i '1h;1!H;${g;s/(mongodb:[[:space:]]*\n[[:space:]]*enable:[[:space:]]*)false/\1true/;p;}' ${CONFIG_MGDB} &>/dev/null
sed -E -n -i '1h;1!H;${g;s/(time-sync:[[:space:]]*\n[[:space:]]*enable:[[:space:]]*)true/\1false/;p;}' ${CONFIG_SIM} &>/dev/null
sed -i "s/estimate-time: true/estimate-time: false/g" ${CONFIG_SIM}
for i in  $(seq 1 $NUM_BENCHMARKS)
do
	DOC_COUNT=$(mongo rcll --eval "db.game_report.count({\"report-name\": \"${NAME_PREFIX}_${i}\"})" --quiet)
	# make sure that the report name is fresh
	if [ ${DOC_COUNT} -ne 0 ]; then
    echo "Skipping ${NAME_PREFIX}_${i} as a report already exists"
	else
		# in rare occasions the generation fails, hence simply retry when it happens
		while [ ${DOC_COUNT} -ne 1 ]
		do
			sed -i "s/store-to-report: \".*\"/store-to-report: \"${NAME_PREFIX}_${i}\"/g" ${CONFIG_GME}
			${LLSF_REFBOX_DIR}/bin/./llsf-refbox &>/dev/null &
			${LLSF_REFBOX_DIR}/bin/./rcll-refbox-instruct -w30
			sleep 1
			echo "refbox started"
			${LLSF_REFBOX_DIR}/bin/./rcll-refbox-instruct -p SETUP -s RUNNING -c Carologistics &>/dev/null
			sleep 2
			echo "instructed setup"
			${LLSF_REFBOX_DIR}/bin/./rcll-refbox-instruct -p PRODUCTION  &>/dev/null
			sleep 2
			echo "instructed production"
			${LLSF_REFBOX_DIR}/bin/./rcll-refbox-instruct -p POST_GAME &>/dev/null
			sleep 1
			read -n 1 -t 15 a
			echo "instructed postgame"
			killall -9 llsf-refbox  &>/dev/null &
			echo "killed refbox instances"
			sleep 1
			DOC_COUNT=$(mongo rcll --eval "db.game_report.count({\"report-name\": \"${NAME_PREFIX}_${i}\"})" --quiet)
		done
		echo "Created report for ${NAME_PREFIX}_${i}"
	fi
done
echo "Restoring original config"
mv ${CONFIG_MGDB}.bak ${CONFIG_MGDB}
mv ${CONFIG_SIM}.bak ${CONFIG_SIM}
mv ${CONFIG_GME}.bak ${CONFIG_GME}
