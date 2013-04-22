
;---------------------------------------------------------------------------
;  machines.clp - LLSF RefBox CLIPS machine processing
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule m-shutdown "Shutdown machines at the end"
  (finalize)
  ?mf <- (machine (name ?m) (desired-lights $?dl&:(> (length$ ?dl) 0)))
  =>
  (modify ?mf (desired-lights))
)

(defrule machine-lights "Set machines if desired lights differ from actual lights"
  ?mf <- (machine (name ?m) (actual-lights $?al) (desired-lights $?dl&:(neq ?al ?dl)))
  =>
  ;(printout t ?m " actual lights: " ?al "  desired: " ?dl crlf)
  (modify ?mf (actual-lights ?dl))
  (foreach ?color (create$ RED YELLOW GREEN)
    (if (member$ (sym-cat ?color "-ON") ?dl)
    then 
      (sps-set-signal (str-cat ?m) ?color "ON")
    else
      (if (member$ (sym-cat ?color "-BLINK") ?dl)
      then
        (sps-set-signal (str-cat ?m) ?color "BLINK")
      else
        (sps-set-signal (str-cat ?m) ?color "OFF")
      )
    )
  )
)

(deffunction machine-init-randomize ()

  ; Gather all available light codes
  (bind ?light-codes (create$))
  (do-for-all-facts ((?lc machine-light-code)) TRUE
    (bind ?light-codes (create$ ?light-codes ?lc:id))
  )
  ; Randomize light codes
  (bind ?light-codes (randomize$ ?light-codes))
  ; Assign random light codes
  (delayed-do-for-all-facts ((?mspec machine-spec)) TRUE
    (do-for-fact ((?light-code machine-light-code)) (= ?light-code:id (nth$ 1 ?light-codes))
      (printout t "Light code " ?light-code:code " for machine type " ?mspec:mtype crlf)
    )
    (modify ?mspec (light-code (nth$ 1 ?light-codes)))
    (bind ?light-codes (delete$ ?light-codes 1 1))
  )

  ; reset machines
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (modify ?machine (loaded-with) (productions 0) (state IDLE)
	             (proc-start 0 0) (puck-id 0) (desired-lights GREEN-ON))
  )

  ; assign random machine types out of the start distribution
  (bind ?machine-assignment (randomize$ ?*MACHINE-DISTRIBUTION*))
  (printout t "Initial machine distribution:    " ?*MACHINE-DISTRIBUTION* crlf)
  (printout t "Randomized machine distribution: " ?machine-assignment crlf)
  (delayed-do-for-all-facts ((?machine machine))
    (any-factp ((?mspec machine-spec)) (eq ?mspec:mtype ?machine:mtype))
    (if (= (length$ ?machine-assignment) 0)
     then (printout logerror "No machine assignment available for " ?machine:name crlf)
     else
       (bind ?mtype (nth$ 1 ?machine-assignment))
       (bind ?machine-assignment (delete$ ?machine-assignment 1 1))
       (printout t "Assigning type " ?mtype " to machine " ?machine:name crlf)
       (modify ?machine (mtype ?mtype))
    )
  )

  ; assign random down times
  (bind ?num-down-times (random 6 8))
  (bind ?candidates (find-all-facts ((?m machine)) ?m:down-possible))
  (loop-for-count (min ?num-down-times (length$ ?candidates))
    (bind ?idx (random 1 (length$ ?candidates)))
    (bind ?duration (random ?*DOWN-TIME-MIN* ?*DOWN-TIME-MAX*))
    (bind ?start-time (random 1 (- ?*PRODUCTION-TIME* ?duration)))
    (bind ?end-time (+ ?start-time ?duration))
    (bind ?mf (nth$ ?idx ?candidates))
    (printout t (fact-slot-value ?mf name) " down from "
	      (time-sec-format ?start-time) " to " (time-sec-format ?end-time)
	      " (" ?duration " sec)" crlf)
    (modify ?mf (down-period ?start-time ?end-time))
    (bind ?candidates (delete$ ?candidates ?idx ?idx))
  )

  ;(printout t "Assigning processing times to machines" crlf)
  (delayed-do-for-all-facts ((?mspec machine-spec)) TRUE
    (bind ?proc-time (random ?mspec:proc-time-min ?mspec:proc-time-max))
    (printout t "Proc time for " ?mspec:mtype " will be " ?proc-time " sec" crlf)
    (modify ?mspec (proc-time ?proc-time))
  )

  (assert (machines-initialized))
)
