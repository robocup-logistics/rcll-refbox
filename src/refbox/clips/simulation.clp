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

(deftemplate sim-time
  (slot enabled (type SYMBOL) (allowed-values false true) (default false))
  (slot estimate (type SYMBOL) (allowed-values false true) (default false))
  (multislot now (type INTEGER) (cardinality 2 2) (default 0 0))
  (multislot last-recv-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (slot real-time-factor (type FLOAT) (default 0.0))
)

;this function returns the time regarding if the simulation-time and the time-estimation is used
(deffunction get-time (?sim-time-sync-enabled ?sim-time-estimate-enabled ?real-time ?sim-time ?last-recv-time ?real-time-factor)
  (if (eq ?sim-time-sync-enabled false)
    then
    (return ?real-time)
    else
    (if (eq ?sim-time-estimate-enabled false)
      then
      (return ?sim-time)
      else
      ;estimate time passed in the simulation
      (bind ?interval (time-diff-sec ?real-time ?last-recv-time))
      (bind ?est-interval (* ?interval ?real-time-factor))
      (return (time-add ?sim-time (time-from-sec ?est-interval)))
    )
  )
)

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