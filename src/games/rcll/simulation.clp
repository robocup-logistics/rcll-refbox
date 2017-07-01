; ---------------------------------------------------------------------------
;  simulation.clp - LLSF RefBox CLIPS rules for working with a simulation

;  Created: Thu Sep 26 19:20:09 2013
;  Copyright  2013  Frederik Zwilling
;  Licensed under BSD license, cf. LICENSE file
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
;  production.clp - LLSF RefBox CLIPS production phase rules

;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
; ---------------------------------------------------------------------------


(defrule sim-init
  (not (sim-time-initialized))
  (confval (path "/llsfrb/simulation/time-sync/estimate-time") (type BOOL) (value ?time-estimate-enable))
  (confval (path "/llsfrb/simulation/time-sync/enable") (type BOOL) (value ?time-sync-enable))
  =>
  (assert (sim-time-initialized)
	  (sim-time (enabled ?time-sync-enable) (estimate ?time-estimate-enable)
		    (now (create$ 0 0)))
  )
)

(defrule sim-net-recv-SimTimeSync
  ?pf <- (protobuf-msg (type "llsf_msgs.SimTimeSync") (ptr ?p) (rcvd-via STREAM))
  ?st <- (sim-time (enabled true) (estimate ?estimate) (now $?old-sim-time)
		   (real-time-factor ?old-rtf) (last-recv-time $?lrt))
  (time $?now)
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (bind ?time-msg (pb-field-value ?p "sim_time"))
  (bind ?sim-time-sec (pb-field-value ?time-msg "sec"))
  (bind ?sim-time-usec (/ (pb-field-value ?time-msg "nsec") 1000))
  (bind ?new-sim-time (create$ ?sim-time-sec ?sim-time-usec))
  ;if the time should be estimated, make sure it stays monoton
  (if (eq ?estimate true)
    then
    (bind ?current-time (get-time true ?estimate ?now ?old-sim-time ?lrt ?old-rtf))
    (bind ?new-sim-time (time-max ?new-sim-time ?current-time))
  )
  (bind ?rtf (pb-field-value ?p "real_time_factor"))
  (modify ?st (now ?new-sim-time) (last-recv-time ?now) (real-time-factor ?rtf))
)

(defrule sim-mps-connect
  (declare (salience ?*PRIORITY_FIRST*))
  ?sf <- (mps (name ?n) (client-id 0))
  =>
  (printout t "sim-connect to mps " ?n " as client 999" crlf)
  (modify ?sf (client-id 999))
)

(defrule sim-mps-reply-small-waiting
  "instead of sending to the actual machines, simulate the responce here"
  (declare (salience ?*PRIORITY_FIRST*))
  (gamestate (game-time ?gt))
  ?c <- (mps-comm-msg (id ?id) (name ?n) (sended-count 0) (game-time ?sat&:(timeout-sec ?gt ?sat 5)) (task ~RESET&~CHANGE-LIGHT&~WAIT-FOR-PICKUP&~WAIT-FOR-PRODUCT))
  =>
  (assert (pb-machine-reply (id ?id) (machine ?n)))
  (modify ?c (sended-count 1))
)

(defrule sim-mps-reply-wait-for-pickup
  "instead of sending to the actual machines, simulate the responce here, TODO: this should be triggert by gazebo"
  (declare (salience ?*PRIORITY_FIRST*))
  (gamestate (game-time ?gt))
  ?c <- (mps-comm-msg (id ?id) (name ?n) (sended-count 0) (task WAIT-FOR-PRODUCT))
  (machine (name ?n) (mps-state AVAILABLE))
  =>
  (assert (pb-machine-reply (id ?id) (machine ?n)))
  (modify ?c (sended-count 1))
)

(defrule sim-mps-reply-direct
  "instead of sending to the actual machines, simulate the responce here"
  (declare (salience ?*PRIORITY_FIRST*))
  ?c <- (mps-comm-msg (id ?id) (name ?n) (sended-count 0) (task RESET|CHANGE-LIGHT))
  =>
  (assert (pb-machine-reply (id ?id) (machine ?n)))
  (modify ?c (sended-count 1))
)

(defrule sim-mps-reply-wait-for-pickup
  "instead of sending to the actual machines, simulate the responce here, TODO: this should be triggert by gazebo"
  (declare (salience ?*PRIORITY_FIRST*))
  (gamestate (game-time ?gt))
  ?c <- (mps-comm-msg (id ?id) (name ?n) (sended-count 0) (task WAIT-FOR-PICKUP))
  (machine (name ?n) (mps-state RETRIEVED))
  =>
  (assert (pb-machine-reply (id ?id) (machine ?n)))
  (modify ?c (sended-count 1))
)
