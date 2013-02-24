
;---------------------------------------------------------------------------
;  machines.clp - LLSF RefBox CLIPS machine processing
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule m-shutdown "Shutdown machines at the end"
  (finalize)
  ?mf <- (machine (name ?m))
  =>
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
  (sps-set-signal (str-cat ?m) "RED" "OFF")
)

