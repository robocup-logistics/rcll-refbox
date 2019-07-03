
;---------------------------------------------------------------------------
;  net.clp - LLSF RefBox CLIPS - keeping tracks of robots in game
;
;  Created: Wed Jun 05 14:19:42 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

; Sort robots by team and name, such that do-for-all-facts on the robot deftemplate
; iterates in a nice order, e.g. for net-send-RobotInfo
(defrule robots-sort
  ?oa <- (robot (number ?number-a) (team-color ?tc-a))
  (or ?ob <- (robot (team-color ?tc-a)
		    (number ?number-b&:(> ?number-a ?number-b)&:(< (fact-index ?oa) (fact-index ?ob))))
      ?oc <- (robot (team-color CYAN&:(eq ?tc-a MAGENTA)&:(< (fact-index ?oa) (fact-index ?oc))))
  )
  =>
  (modify ?oa)
)

(defrule robot-lost
  (time $?now)
  ?rf <- (robot (number ?number) (team ?team) (name ?name) (host ?host) (port ?port)
		(warning-sent FALSE) (last-seen $?ls&:(timeout ?now ?ls ?*PEER-LOST-TIMEOUT*)))
  =>
  (modify ?rf (warning-sent TRUE))
  (printout warn "Robot " ?number " " ?name "/" ?team " at " ?host " lost" crlf)
  (assert (attention-message (team ?team)
			     (text (str-cat "Robot " ?number " " ?name "/" ?team
					    " at " ?host " lost"))))
)

(defrule robot-remove
  (time $?now)
  ?rf <- (robot (number ?number) (team ?team) (name ?name) (host ?host) (port ?port)
		(last-seen $?ls&:(timeout ?now ?ls ?*PEER-REMOVE-TIMEOUT*)))
  =>
  (retract ?rf)
  (printout warn "Robot " ?number " " ?name "/" ?team " at " ?host " definitely lost" crlf)
  (assert
   (attention-message (text (str-cat "Robot " ?number " " ?name "/" ?team
				     " at " ?host " definitely lost"))))
)

(defrule robot-maintenance-warning
  (gamestate (cont-time ?ctime))
  ?rf <- (robot (state MAINTENANCE) (number ?number) (name ?name) (team ?team)
		(maintenance-warning-sent FALSE)
		(maintenance-start-time ?st&:(timeout-sec ?ctime ?st ?*MAINTENANCE-WARN-TIME*)))

  =>
  (modify ?rf (maintenance-warning-sent TRUE))
  (assert (attention-message (team ?team)
			     (text (str-cat "Robot " ?number " " ?name "/" ?team
					    " maintenance almost over"))))
)

(defrule robot-disqualify-maintenance-timeout
  (gamestate (cont-time ?ctime))
  ?rf <- (robot (state MAINTENANCE) (number ?number) (name ?name) (team ?team)
		(maintenance-start-time ?st&:(timeout-sec ?ctime ?st (+ ?*MAINTENANCE-ALLOWED-TIME* ?*MAINTENANCE-GRACE-TIME*))))

  =>
  (modify ?rf (state DISQUALIFIED))
  (printout warn "Disqualifying robot " ?number " " ?name "/" ?team " for maintenance timeout" crlf)
  (assert (attention-message (team ?team)
			     (text (str-cat "Robot " ?number " " ?name "/" ?team
					    " disqualified (maintenance timeout)"))))
)

(defrule robot-disqualify-maintenance-cycles
  ?rf <- (robot (number ?number) (name ?name) (team ?team)
		(state ~DISQUALIFIED)
		(maintenance-cycles ?mc&:(> ?mc ?*MAINTENANCE-ALLOWED-CYCLES*)))
  =>
  (modify ?rf (state DISQUALIFIED))
  (printout warn "Disqualifying robot " ?number " " ?name "/" ?team
	    " for too many maintenance cycles" crlf)
  (assert (attention-message (team ?team)
			     (text (str-cat "Robot " ?number " " ?name "/" ?team
					    " disqualified (maintenance cycles)"))))
)

(defrule robot-recv-SetRobotMaintenance
  ?pf <- (protobuf-msg (type "llsf_msgs.SetRobotMaintenance") (ptr ?p) (rcvd-via STREAM))
  (gamestate (phase ?phase) (cont-time ?ctime) (game-time ?game-time))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (do-for-fact ((?robot robot))
    (and (eq ?robot:number (pb-field-value ?p "robot_number"))
	 (eq ?robot:team-color (sym-cat (pb-field-value ?p "team_color"))))

    (if (eq (pb-field-value ?p "maintenance") TRUE)
    then
      (if (eq ?robot:state ACTIVE) then
	(bind ?cycle (+ ?robot:maintenance-cycles 1))
	(printout t "Robot " ?robot:number " scheduled for maintenance cycle " ?cycle crlf)
	(modify ?robot (state MAINTENANCE) (maintenance-start-time ?ctime)
		(maintenance-cycles ?cycle) (maintenance-warning-sent FALSE))
	(bind ?cycle-cost (nth$ ?cycle ?*MAINTENANCE-COST*))
	(if (neq ?cycle-cost nil)
	 then
		(assert (points (game-time ?game-time)
		                (points (* -1 ?cycle-cost))
		                (team ?robot:team-color) (phase ?phase)
		                (reason (str-cat "Maintenance of robot " ?robot:number))))
	)
      )
    else
      (bind ?maint-time (- ?ctime ?robot:maintenance-start-time))
      (if (<= ?maint-time (+ ?*MAINTENANCE-ALLOWED-TIME* ?*MAINTENANCE-GRACE-TIME*))
      then
        (printout t "Robot " ?robot:number " back from maintenance" crlf)
        (if (> ?maint-time ?*MAINTENANCE-ALLOWED-TIME*)
	  then (printout t "Robot maintenance grace time granted" crlf))
	(if (> ?robot:maintenance-cycles ?*MAINTENANCE-ALLOWED-CYCLES*)
	  then (printout t "Reviving disqualified robot" crlf))
        (modify ?robot (state ACTIVE)
		(maintenance-cycles (min ?*MAINTENANCE-ALLOWED-CYCLES* ?robot:maintenance-cycles)))
      else
        (printout t "Denying re-entering of robot " ?robot:number
		  " (maintenance time exceeded over grace time)" crlf)
      )
    )
  )
)


(defrule robot-beacon-known
  ?bf <- (robot-beacon (time $?t) (rcvd-at $?rcvd-at) (number ?number) (team-name ?team-name)
		       (team-color ?team-color) (peer-name ?peer-name)
		       (host ?host) (port ?port)
		       (has-pose ?has-pose) (pose $?pose) (pose-time $?pose-time))
  ?rf <- (robot (number ?number) (team ?team-name) (team-color ?r-team-color) (name ?r-name)
		(host ?r-host) (port ?r-port))
  =>
  (retract ?bf)

  (if (neq ?team-color ?r-team-color) then
    (assert (attention-message (text (str-cat "Robot " ?peer-name " of " ?team-name
					      " has changed team color from("
					      ?r-team-color " to " ?team-color))))
  )
  (if (neq ?r-name ?peer-name) then
    (assert (attention-message (text (str-cat "Robot " ?number " of " ?team-name
					      " has changed name from " ?r-name
					      " to " ?peer-name))))
  )
  (modify ?rf (warning-sent FALSE) (last-seen ?rcvd-at)
	  (name ?peer-name) (team-color ?team-color) (host ?host) (port ?port)
	  (has-pose ?has-pose) (pose ?pose) (pose-time ?pose-time))
)


(defrule robot-beacon-unknown
  ?bf <- (robot-beacon (time $?t) (rcvd-at $?rcvd-at) (number ?number) (team-name ?team-name)
		       (team-color ?team-color) (peer-name ?peer-name)
		       (host ?host) (port ?port)
		       (has-pose ?has-pose) (pose $?pose) (pose-time $?pose-time))
  (not (robot (number ?number) (team ?team-name)))
  ?sf <- (signal (type version-info))
  =>
  (retract ?bf)
  (modify ?sf (count 0) (time 0 0))

  (printout debug "Received initial beacon from " ?peer-name " of " ?team-name
	    "(" ?host ":" ?port ")" crlf)

  (bind ?peer-time-diff (abs (time-diff-sec ?rcvd-at ?t)))
  (if (> ?peer-time-diff ?*PEER-TIME-DIFFERENCE-WARNING*) then
    (assert (attention-message (text (str-cat "Robot " ?peer-name " of " ?team-name
					      " has a large time offset ("
					      ?peer-time-diff " sec)"))))
  )
  (if (= ?number 0) then
    (assert (attention-message (text (str-cat "Robot " ?peer-name "(" ?team-name
					      ") has jersey number 0"
					      " (" ?host ":" ?port ")"))))
  )

  (if (eq ?team-color nil) then
    (assert (attention-message (text (str-cat "Robot " ?peer-name "(" ?team-name
					      ") has does not provide its team color"
					      " (" ?host ":" ?port ")"))))
  )

  (if (and (eq ?team-name "LLSF") (eq ?peer-name "RefBox")) then
    (assert (attention-message (text (str-cat "Detected another RefBox at "
					      ?host ":" ?port))))
  )

  (assert (robot (state ACTIVE) (warning-sent FALSE) (last-seen ?rcvd-at)
		 (number ?number) (team ?team-name)
		 (name ?peer-name) (team-color ?team-color) (host ?host) (port ?port)
		 (has-pose ?has-pose) (pose ?pose) (pose-time ?pose-time)))
)
