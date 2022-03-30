;---------------------------------------------------------------------------
;  exploration.clp - LLSF RefBox CLIPS exploration report processing
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
;             2022  Tarik Viehmann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------


(defrule exploration-report-incoming
	(gamestate (phase EXPLORATION|PRODUCTION) (game-time ?game-time))
  ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
	                     (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
	=>
	(retract ?mf)
	(bind ?team (sym-cat (pb-field-value ?p "team_color")))
	(foreach ?m (pb-field-list ?p "machines")
		(bind ?name (sym-cat (pb-field-value ?m "name")))
		(bind ?zone (if (pb-has-field ?m "zone") then (sym-cat (pb-field-value ?m "zone")) else NOT-REPORTED))
		(bind ?rotation (if (pb-has-field ?m "rotation") then (pb-field-value ?m "rotation") else -1))
		(assert (exploration-report (rtype INCOMING)
		                            (name ?name) (zone ?zone) (rotation ?rotation)
		                            (game-time ?game-time)
		                            (team ?team) (host ?from-host) (port ?from-port)))
		(pb-destroy ?m)
	)
)

(defrule exploration-report-incoming-cleanup
	(declare (salience ?*PRIORITY_CLEANUP*))
	?ei <- (exploration-report (rtype INCOMING))
	=>
	(retract ?ei)
)

(defrule exploration-report-new
	(exploration-report (rtype INCOMING) (name ?name) (zone ?zone) (rotation ?rotation)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
	(not (exploration-report (rtype RECORD) (name ?name)))
	=>
	(assert (exploration-report (rtype RECORD)
	                            (name ?name) (zone NOT-REPORTED) (rotation -1)
	                            (game-time ?game-time)
	                            (team ?team) (host ?from-host) (port ?from-port)))
)

(defrule exploration-report-rotation-correct
	; only triggers after zone-state is CORRECT_REPORT
	(exploration-report (rtype INCOMING) (name ?name) (rotation ?rotation&~-1)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (name ?name)
	                           (zone-state CORRECT_REPORT) (rotation -1))
	(machine (name ?name) (team ?team) (rotation ?rotation))
	=>
  (modify ?er (rotation ?rotation) (rotation-state CORRECT_REPORT))
	(printout t "Correct partial report: " ?name " (rotation " ?rotation "). "
	            "Awarding " ?*EXPLORATION-CORRECT-REPORT-ROTATION-POINTS* " points" crlf)
	(assert (points (points ?*EXPLORATION-CORRECT-REPORT-ROTATION-POINTS*)
	                (phase EXPLORATION) (team ?team) (game-time ?game-time)
	                (reason (str-cat "Correct partial exploration report for "
	                                 ?name ": rotation = " ?rotation))))
)

(defrule exploration-report-rotation-wrong
	; only triggers after zone-state is CORRECT_REPORT
	(exploration-report (rtype INCOMING) (name ?name) (rotation ?rotation&~-1)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (name ?name)
	                           (zone-state CORRECT_REPORT) (rotation -1))
	?mf <- (machine (name ?name) (team ?team) (rotation ?erotation&~?rotation))
	=>
	(modify ?er (rotation ?rotation) (rotation-state WRONG_REPORT))
	(printout t "Wrong partial report: " ?name " (rotation " ?rotation "). "
	          "Awarding " ?*EXPLORATION-WRONG-REPORT-ROTATION-POINTS* " points" crlf)
	(assert (points (points ?*EXPLORATION-WRONG-REPORT-ROTATION-POINTS*)
	                (phase EXPLORATION) (team ?team) (game-time ?game-time)
	                (reason (str-cat "Wrong partial exploration report for "
	                        ?name ": rotation = " ?rotation " (should be " ?erotation ")"))))
)

(defrule exploration-report-rotation-ignored
	; triggers when zone-state is WRONG_REPORT
	(exploration-report (rtype INCOMING) (name ?name) (rotation ?rotation&~-1)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
  ?er <- (exploration-report (rtype RECORD) (name ?name) (zone-state WRONG_REPORT) (rotation -1))
	(machine (name ?name) (team ?team))
	=>
  (modify ?er (rotation ?rotation))
  (printout t "Ignored partial report: " ?name " (rotation " ?rotation "). since zone is wrong" crlf)
)

(defrule exploration-report-zone-correct
	(exploration-report (rtype INCOMING) (name ?name) (zone ?zone&~NOT-REPORTED)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (name ?name) (zone NOT-REPORTED))
	?mf <- (machine (name ?name) (team ?team) (zone ?zone))
	=>
	(modify ?er (zone ?zone) (zone-state CORRECT_REPORT))
	(printout t "Correct partial report: " ?name " (zone " ?zone "). "
	          "Awarding " ?*EXPLORATION-CORRECT-REPORT-ZONE-POINTS* " points." crlf)
	(assert (points (points ?*EXPLORATION-CORRECT-REPORT-ZONE-POINTS*)
	                (phase EXPLORATION) (team ?team) (game-time ?game-time)
	                (reason (str-cat "Correct partial exploration report for "
	                        ?name ": zone = " ?zone))))
)

(defrule exploration-report-zone-wrong
	(exploration-report (rtype INCOMING) (name ?name) (zone ?zone&~NOT-REPORTED)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (name ?name) (zone NOT-REPORTED))
	?mf <- (machine (name ?name) (team ?team) (zone ?mzone&~?zone))
	=>
	(modify ?er (zone ?zone) (zone-state WRONG_REPORT))
	(printout t "Wrong partial report: " ?name " (zone " ?zone "). "
	          "Awarding " ?*EXPLORATION-WRONG-REPORT-ZONE-POINTS* " points" crlf)
	(assert (points (points ?*EXPLORATION-WRONG-REPORT-ZONE-POINTS*)
	                (phase EXPLORATION) (team ?team) (game-time ?game-time)
	                (reason (str-cat "Wrong partial exploration report for "
	                        ?name ": zone = " ?zone " (should be " ?mzone ")"))))
)

(defrule exploration-report-complete-correct
	?er <- (exploration-report (rtype RECORD) (correctly-reported UNKNOWN)
	                           (name ?name) (zone ?zone&~NOT-REPORTED) (rotation ?rotation&~-1) )
	?mf <- (machine (name ?name) (team ?team) (zone ?zone) (rotation ?rotation))
	=>
	(modify ?er (correctly-reported TRUE))
)

(defrule exploration-report-complete-zone-wrong
	?er <- (exploration-report (rtype RECORD) (correctly-reported UNKNOWN)
	                           (name ?name) (zone ?zone&~NOT-REPORTED))
	?mf <- (machine (name ?name))
	(or
		(machine (name ?name) (team ?team) (zone ~?zone))
	)
	=>
	(modify ?er (correctly-reported FALSE))
)

(defrule exploration-cleanup-report
	(gamestate (phase ~EXPLORATION))
	?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
	       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
	=>
	(retract ?mf)
)

(defrule exploration-send-MachineReportInfo
	(time $?now)
	(gamestate (phase EXPLORATION|PRODUCTION) (game-time ?game-time&:(< ?game-time ?*EXPLORATION-TIME*)))
	?sf <- (signal (type machine-report-info)
	       (time $?t&:(timeout ?now ?t ?*BC-MACHINE-REPORT-INFO-PERIOD*)) (seq ?seq))
	(network-peer (group CYAN) (id ?peer-id-cyan))
	(network-peer (group MAGENTA) (id ?peer-id-magenta))
	=>
	(modify ?sf (time ?now) (seq (+ ?seq 1)))

	; CYAN
	(bind ?s (pb-create "llsf_msgs.MachineReportInfo"))

	(pb-set-field ?s "team_color" CYAN)
	(do-for-all-facts ((?report exploration-report)) (and (eq ?report:team CYAN) (eq ?report:rtype RECORD))
		(pb-add-list ?s "reported_machines" ?report:name)
	)

	(pb-broadcast ?peer-id-cyan ?s)
	(pb-destroy ?s)

	; MAGENTA
	(bind ?s (pb-create "llsf_msgs.MachineReportInfo"))

	(pb-set-field ?s "team_color" MAGENTA)
	(do-for-all-facts ((?report exploration-report)) (and (eq ?report:team MAGENTA) (eq ?report:rtype RECORD))
		(pb-add-list ?s "reported_machines" ?report:name)
	)

	(pb-broadcast ?peer-id-magenta ?s)
	(pb-destroy ?s)
)

(defrule exploration-set-ground-truth
	(machine (name ?n) (team ?team) (rotation ?rotation) (zone ?zone))
	(not (exploration-report (name ?n) (rtype RECORD) (correctly-reported TRUE)))
	(gamestate (phase PRODUCTION) (game-time ?game-time&:(>= ?game-time ?*EXPLORATION-TIME*)))
	=>
	(do-for-all-facts ((?exp exploration-report))
		(retract ?exp)
	)
	(do-for-all-facts ((?m machine))
		(assert (exploration-report (name ?m:name) (rtype RECORD) (zone ?zone)
		                            (rotation ?rotation) (correctly-reported TRUE)))
	)
)
