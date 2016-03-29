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


(defrule sim-init-time
  (not (sim-time-initialized))
  (confval (path "/llsfrb/simulation/time-sync/estimate-time") (type BOOL) (value ?time-estimate-enable))
  (confval (path "/llsfrb/simulation/time-sync/enable") (type BOOL) (value ?time-sync-enable))
  =>
  (assert (sim-time-initialized)
	  (sim-time (enabled ?time-sync-enable) (estimate ?time-estimate-enable)
		    (now (create$ 0 0)))
  )
)

(defrule sim-init-default-map
  (not (sim-init-default-map))
  (confval (path "/llsfrb/simulation/use-default-map") (type BOOL) (value true))
  =>
	(printout warn "Using default simulation map, setting machine zones accordingly" crlf)
  (assert (sim-init-default-map))
  (do-for-fact ((?m machine)) (eq ?m:name C-BS)  (modify ?m (zone Z9)))
  (do-for-fact ((?m machine)) (eq ?m:name C-DS)  (modify ?m (zone Z4)))
  (do-for-fact ((?m machine)) (eq ?m:name C-CS1)  (modify ?m (zone Z24)))
  (do-for-fact ((?m machine)) (eq ?m:name C-CS2)  (modify ?m (zone Z18)))
  (do-for-fact ((?m machine)) (eq ?m:name C-RS1)  (modify ?m (zone Z2)))
  (do-for-fact ((?m machine)) (eq ?m:name C-RS2)  (modify ?m (zone Z11)))

  (do-for-fact ((?m machine)) (eq ?m:name M-BS)  (modify ?m (zone Z21)))
  (do-for-fact ((?m machine)) (eq ?m:name M-DS)  (modify ?m (zone Z16)))
  (do-for-fact ((?m machine)) (eq ?m:name M-CS1)  (modify ?m (zone Z12)))
  (do-for-fact ((?m machine)) (eq ?m:name M-CS2)  (modify ?m (zone Z6)))
  (do-for-fact ((?m machine)) (eq ?m:name M-RS1)  (modify ?m (zone Z14)))
  (do-for-fact ((?m machine)) (eq ?m:name M-RS2)  (modify ?m (zone Z23)))
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

(defrule sim-mps-processed
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  (machine (name ?n) (state PROCESSING) (bases-added ?ba))
  =>
  (assert (machine-mps-state (name ?n) (state PROCESSED) (num-bases ?ba)))
)

(deffunction mps-reset (?name)
  ;(bind ?name (sym-cat ?name))
  ;(printout t "Simulated machine reset" crlf)
)

(deffunction mps-reset-base-counter (?name)
  (bind ?mname (sym-cat ?name))
  ;(printout t "Reset base counter for " ?name crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?mname)
    (assert (machine-mps-state (name ?mname) (state ?m:state) (num-bases 0)))
  )
)

(deffunction mps-set-light (?name ?color ?state)
  ;(bind ?name (sym-cat ?name))
  ;(printout t "Simulated light setting for " ?name ": " ?color "/" ?state crlf)
)

(deffunction mps-set-lights (?name ?state-red ?state-yellow ?state-green)
  (bind ?name (sym-cat ?name))
  (printout t "Simulated light setting for " ?name ": red=" ?state-red
						" yellow=" ?state-yellow "  green=" ?state-green crlf)
)

(deffunction mps-bs-dispense (?name ?color ?side)
  (bind ?name (sym-cat ?name))
  (printout t "Simulated dispense at " ?name " for " ?color " at " ?side crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?name)
    (assert (machine-mps-state (name ?name) (state PROCESSED)))
  )
)
(deffunction mps-ds-process (?name ?gate)
  (bind ?name (sym-cat ?name))
  (printout t "Simulated delivery at " ?name " on lane " ?gate crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?name)
    (assert (machine-mps-state (name ?name) (state PROCESSED)))
  )
)

(deffunction mps-rs-mount-ring (?name ?color)
  (bind ?name (sym-cat ?name))
  (printout t "Simulated ring mounting " ?name " for " ?color crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?name)
    (assert (machine-mps-state (name ?name) (state PROCESSED) (num-bases ?m:bases-added)))
  )
)

(deffunction mps-cs-process (?name ?cs-op)
  (bind ?name (sym-cat ?name))
  (printout t "Simulated  " ?cs-op " at " ?name crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?name)
    (assert (machine-mps-state (name ?name) (state PROCESSED)))
  )
)

(deffunction mps-deliver (?name)
  (bind ?name (sym-cat ?name))
  (printout t "Simulated output at " ?name crlf)
  ;(do-for-fact ((?m machine)) (eq ?m:name ?name)
  ;  (assert (machine-mps-state (name ?name) (state DELIVERED) (num-bases ?m:bases-added)))
  ;)
)
