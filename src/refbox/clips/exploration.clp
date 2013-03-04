
;---------------------------------------------------------------------------
;  exploration.clp - LLSF RefBox CLIPS exploration phase rules
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule machine-enable-exploration
  ?gf <- (gamestate (phase EXPLORATION) (prev-phase ~EXPLORATION))
  ?mf <- (machine (mtype ?mtype))
  (machine-spec (mtype ?mtype) (light-code ?lc))
  =>
  (modify ?gf (prev-phase EXPLORATION))
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (bind ?dl (create$))
    (do-for-fact ((?spec machine-spec)) (eq ?machine:mtype ?spec:mtype)
      (bind ?dl ?spec:light-code)
    )
    (modify ?machine (desired-lights ?dl))
  )
)


(defrule exploration-handle-report
  ?gf <- (gamestate (phase EXPLORATION) (points ?points) (game-time ?game-time))
  ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  =>
  (retract ?mf)
  (foreach ?m (pb-field-list ?p "machines")
    (bind ?name (sym-cat (pb-field-value ?m "name")))
    (bind ?type (sym-cat (pb-field-value ?m "type")))
    (do-for-fact ((?machine machine)) (eq ?machine:name ?name)
      (if (eq ?machine:mtype ?type)
        then
          ; correct report, has it already been reported?
          (if (not (any-factp ((?report exploration-report)) (eq ?report:name ?name)))
	    then
              (printout t "Correct report: " ?name " of type " ?type crlf) 
              (modify ?gf (points (+ ?points ?*EXPLORATION-CORRECT-REPORT-POINTS*)))
	      (assert (exploration-report (name ?name) (type ?type) (game-time ?game-time)
					  (host ?from-host) (port ?from-port)))
          )
        else
          (printout t "Invalid report: " ?name " of type " ?type crlf) 
          (modify ?gf (points (max (+ ?points ?*EXPLORATION-INVALID-REPORT-POINTS*) 0)))
      )
    )
    (pb-destroy ?m)
  )
)

(defrule exploration-cleaup-report
  (gamestate (phase ~EXPLORATION))
  ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  =>
  (retract ?mf)
)

(defrule exploration-send-MachineReportInfo
  (time $?now)
  (gamestate (phase EXPLORATION))
  ?sf <- (signal (type machine-report-info)
		 (time $?t&:(timeout ?now ?t ?*BC-MACHINE-REPORT-INFO-PERIOD*)) (seq ?seq))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)))
  (bind ?s (pb-create "llsf_msgs.MachineReportInfo"))

  (do-for-all-facts ((?report exploration-report)) TRUE
    (pb-add-list ?s "reported_machines" ?report:name)
  )

  (pb-broadcast ?s)
  (pb-destroy ?s)
)
