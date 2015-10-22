
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
  (declare (salience ?*PRIORITY_HIGH*))
  ?oa <- (robot (number ?number-a))
  ?ob <- (robot (number ?number-b&:(> ?number-a ?number-b)&:(< (fact-index ?oa) (fact-index ?ob))))
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
  (gamestate (cont-time ?ctime))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (do-for-fact ((?robot robot))
    (and (eq ?robot:number (pb-field-value ?p "robot_number"))
	 (eq ?robot:team-color (sym-cat (pb-field-value ?p "team_color"))))

    (if (= (pb-field-value ?p "maintenance") 1)
    then
      (if (eq ?robot:state ACTIVE) then
	(bind ?cycle (+ ?robot:maintenance-cycles 1))
	(printout t "Robot " ?robot:number " scheduled for maintenance cycle " ?cycle crlf)
	(modify ?robot (state MAINTENANCE) (maintenance-start-time ?ctime)
		(maintenance-cycles ?cycle) (maintenance-warning-sent FALSE))
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
