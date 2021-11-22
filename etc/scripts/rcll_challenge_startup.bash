#! /bin/bash
#
# rcll_challenge_startup.bash start and configure the refbox to play challenges
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

usage()
{
cat << EOF
usage: $0 <options> [-- <refbox-options>]

This script automatically configures the RefBox to run challenges

The following refbox options are set per default:
--cfg-mps mps/mockup_mps.yaml
--cfg-mongodb mongodb/enable_mongodb.yaml


Options:
   -h                              Show this message

Exactly one of the following options is required:
   --production [c0|c1|c2|c3]      Production Challenge (sets --cfg-challenges)
   --navigation                    Navigation challenge (sets --cfg-challenges)
   --exploration                   Exploration challenge (sets --cfg-challenges)
   --grasping                      Grasping challenge
                                   (sets --cfg-challenges and --cfg-game)
                                   Make sure that
                                   benchmarks/grasping_challenge.gz is
                                   loaded to your mongodb instance

Additional tweaking:
   --ground-truth                  Send Ground Truth
                                   (ignored, unless --production is used)
   --difficulty [easy|medium|hard]
                                   (ignored, unless --navigation or
                                    --exploration is used)
   --pack-results                  Pack the logfiles and mongodb game report
                                   to an archive

Any refbox option set via this wrapper script can be overridden
using <refbox-options>, if necessary.
EOF
$LLSF_REFBOX_DIR/bin/./llsf-refbox -h
}

PACK_RESULTS=
GAME_CFG=
CHALLENGE_OPT=" --cfg-challenges "
MPS_CFG=" --cfg-mps mps/mockup_mps.yaml "
CHALLENGE_FILE=
CHALLENGE_SUFFIX=
CHALLENGE_FOLDER=
MONGODB_CFG=" --cfg-mongodb mongodb/enable_mongodb.yaml"
OPTS=$(getopt -o "h" -l "production:,ground-truth,navigation,exploration,grasping,difficulty:,pack-results" -- "$@")

if [ -z "$LLSF_REFBOX_DIR" ]; then
	echo "LLSF_REFBOX_DIR not set, abort"
	exit
fi

if [ $? != 0 ]
then
	echo "Failed to parse parameters"
	usage
	exit 1
fi

eval set -- "$OPTS"
while true; do
	OPTION=$1
	OPTARG=$2
	case $OPTION in
		-h)
			usage
			exit 1
			;;
		--production)
			if [ -n "$CHALLENGE_FILE" ]; then
				echo "Can only use one challenge at a time!"
				exit
			fi
			case $OPTARG in
				c0)
					CHALLENGE_FILE="c0"
					;;
				c1)
					CHALLENGE_FILE="c1"
					;;
				c2)
					CHALLENGE_FILE="c2"
					;;
				c3)
					CHALLENGE_FILE="c3"
					;;
				*)
					echo "Option --production expected one of c0, c1, c2 or c3,\
					      instead got " $OPTARG ". Abort"
					;;
			esac
			if [ -z "$CHALLENGE_FOLDER" ]; then
				CHALLENGE_FOLDER="challenges/prod_no_gt/"
			fi
			CHALLENGE_SUFFIX=".yaml"
			;;
		--exploration)
			if [ -n "$CHALLENGE_FILE" ]; then
				echo "Can only use one challenge at a time!"
				exit
			fi
			CHALLENGE_FILE="exploration"
			CHALLENGE_FOLDER="challenges/exp/"
			;;
		--navigation)
			if [ -n "$CHALLENGE_FILE" ]; then
				echo "Can only use one challenge at a time!"
				exit
			fi
			CHALLENGE_FILE="nav"
			CHALLENGE_FOLDER="challenges/nav/"
			;;
		--grasping)
			if [ -n "$CHALLENGE_FILE" ]; then
				echo "Can only use one challenge at a time!"
				exit
			fi
			CHALLENGE_FILE="grasping"
			CHALLENGE_FOLDER="challenges/grasping/"
			GAME_CFG=" --cfg-game challenges/grasping/grasping_game.yaml "
			CHALLENGE_SUFFIX=".yaml"
			;;
		--difficulty)
			case $OPTARG in
				easy)
					CHALLENGE_SUFFIX="_easy.yaml"
					;;
				medium)
					CHALLENGE_SUFFIX="_medium.yaml"
					;;
				hard)
					CHALLENGE_SUFFIX="_hard.yaml"
					;;
				*)
					echo "Option --difficulty expected one of easy, medium or hard,\
					      instead got " $OPTARG ". Abort"
					;;
			esac
			;;
		--ground-truth)
			CHALLENGE_FOLDER="challenges/prod/"
		;;
		--pack-results)
			PACK_RESULTS=1
			;;
		--)
			shift
			break
			;;
		esac
		shift
done



if [ -z "$CHALLENGE_FILE" ]; then
	echo "No challenge selected, abort"
	usage
	exit
fi
if [ -z "$CHALLENGE_SUFFIX" ]; then
	echo "No difficulty selected, abort"
	usage
	exit
fi




if [ -n "$PACK_RESULTS" ]; then
		stop_run () {
		DATE=$(date "+%d_%m_%Y-%H_%M_%S")
	  trap - $TRAP_SIGNALS
		LATEST_REPORT=$(mongo --quiet rcll --eval "db.game_report.find('', {'_id':1}).sort({_id:-1}).limit(1)")
		mongodump -d rcll -c game_report -q "$LATEST_REPORT" --gzip --archive=mongo_game_report.gz
		tar -chzf output_$DATE.tar.gz mongo_game_report.gz refbox-debug_latest.log refbox_latest.log
		rm mongo_game_report.gz
	}
	TRAP_SIGNALS="SIGINT SIGTERM SIGPIPE EXIT"
	trap stop_run $TRAP_SIGNALS
fi

$LLSF_REFBOX_DIR/bin/./llsf-refbox $CHALLENGE_OPT \
	$CHALLENGE_FOLDER$CHALLENGE_FILE$CHALLENGE_SUFFIX \
	$MONGODB_CFG $MPS_CFG $GAME_CFG $@
