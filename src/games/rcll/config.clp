
;---------------------------------------------------------------------------
;  config.clp - CLIPS agent configuration tools
;
;  Created: Wed Dec 19 20:45:53 2012 (Train from Munich to Freiburg)
;  Copyright  2012  Tim Niemueller [www.niemueller.de]
;  Licensed under GPLv2+ license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate confval
  (slot path (type STRING))
  (slot type (type SYMBOL) (allowed-values FLOAT UINT INT BOOL STRING))
  (slot value)
  (slot is-list (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
  (multislot list-value)
)

(defrule config-print-disabled-slide-counter-checks
  (confval (path "/llsfrb/simulation/disable-base-payment-check")
					 (is-list TRUE) (list-value $?disabled-machines))
	=>
  (do-for-all-facts ((?m machine))
      (and (eq ?m:mtype RS)
					 (member$ (str-cat ?m:name) ?disabled-machines))
    (printout warn "Slide counter check for "
                   (str-cat ?m:name) " disabled, by config" crlf))
)

(defrule config-print-invalid-slide-counter-check-option
  (confval (path "/llsfrb/simulation/disable-base-payment-check")
					 (is-list TRUE) (list-value $? ?m $?))
	(not (machine (name ?machine&:(eq ?machine (sym-cat ?m))) (mtype RS)))
	=>
  (printout warn "Disabling of slide counter check on machine " ?m " failed. "
                 "Reason: " ?m " is not a ring station" crlf)
)

;(defrule print-confval
;  (confval (path ?p) (type ?t) (value ?v) (is-list ?is-list) (list-value $?lv))
;  =>
;  (if (debug 2) then
;    (printout t "confval path: " ?p "  type: " ?t
;	      "  value: " (if (eq ?is-list TRUE) then ?lv else ?v) crlf))
;)
