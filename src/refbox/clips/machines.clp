
;---------------------------------------------------------------------------
;  machines.clp - LLSF RefBox CLIPS machine processing
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction any-puck-in-state (?puck-state $?puck-ids)
  (foreach ?id ?puck-ids
    (if (any-factp ((?puck puck)) (and (eq ?puck:id ?id) (eq ?puck:state ?puck-state)))
      then (return TRUE)))
  (return FALSE)
)

(defrule m-shutdown "Shutdown machines at the end"
  (finalize)
  ?mf <- (machine (name ?m) (desired-lights $?dl&:(> (length$ ?dl) 0)))
  =>
  (modify ?mf (desired-lights))
)

(defrule machine-lights
  ?mf <- (machine (name ?m) (actual-lights $?al) (desired-lights $?dl&:(neq ?al ?dl)))
  =>
  (printout t ?m " actual lights: " ?al "  desired: " ?dl crlf)
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
