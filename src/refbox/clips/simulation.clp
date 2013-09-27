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
  ?st <- (sim-time (enabled true))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (bind ?sim-time (pb-field-value ?p "sim_time"))
  (bind ?sim-time-sec (pb-field-value ?sim-time "sec"))
  (bind ?sim-time-usec (/ (pb-field-value ?sim-time "nsec") 1000))
  (bind ?rtf (pb-field-value ?p "real_time_factor"))
  (modify ?st (now (create$ ?sim-time-sec ?sim-time-usec)))
)

(deffunction get-time (?sim-time-sync-enabled ?sim-time-estimate-enabled ?real-time ?sim-time)
  (if (eq ?sim-time-sync-enabled false)
    then
    (return ?real-time)
    else
    ;TODO: time estimation with sim-time-speed
    (return ?sim-time)
  )
)