
;---------------------------------------------------------------------------
;  refbox.clp - LLSF RefBox CLIPS main file
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(load* (resolve-file net.clp))
(load* (resolve-file machines.clp))
(load* (resolve-file orders.clp))
(load* (resolve-file production.clp))
(load* (resolve-file exploration.clp))

(defrule update-gametime
  (declare (salience ?*PRIORITY_FIRST*))
  (time $?now)
  ?gf <- (gamestate (state RUNNING)
		    (game-time ?game-time) (last-time $?last-time&:(neq ?last-time ?now)))
  =>
  (modify ?gf (game-time (+ ?game-time (time-diff-sec ?now ?last-time))) (last-time ?now))
)

(defrule update-last-time
  (declare (salience ?*PRIORITY_FIRST*))
  (time $?now)
  ?gf <- (gamestate (state ~RUNNING)
		    (last-time $?last-time&:(neq ?last-time ?now)))
  =>
  (modify ?gf (last-time ?now))
)

(defrule rfid-input-cleanup
  (declare (salience ?*PRIORITY_CLEANUP*))
  ?f <- (rfid-input (machine ?m) (has-puck ?hp) (id ?id))
  =>
  (retract ?f)
  ;(if (debug 1) then
  ;  (printout t "Clearing unused RFID input (" ?m ", " ?hp ", " ?id ")" crlf))
)


(defrule silence-debug-facts
  (declare (salience -1000))
  (init)
  (confval (path "/llsfrb/clips/debug") (type BOOL) (value true))
  (confval (path "/llsfrb/clips/unwatch-facts") (type STRING) (is-list TRUE) (list-value $?lv))
  =>
  (printout t "Disabling watching of the following facts: " ?lv crlf)
  (foreach ?v ?lv (unwatch facts (sym-cat ?v)))
)

(defrule silence-debug-rules
  (declare (salience -1000))
  (init)
  (confval (path "/llsfrb/clips/debug") (type BOOL) (value true))
  (confval (path "/llsfrb/clips/unwatch-rules") (type STRING) (is-list TRUE) (list-value $?lv))
  =>
  (printout t "Disabling watching of the following rules: " ?lv crlf)
  (foreach ?v ?lv (unwatch rules (sym-cat ?v)))
)



(defrule init-game
  ?gf <- (gamestate (state INIT))
  =>
  (modify ?gf (state RUNNING) (last-time (now)))
)

