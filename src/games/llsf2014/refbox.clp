
;---------------------------------------------------------------------------
;  refbox.clp - LLSF RefBox CLIPS main file
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

; LLSF RefBox Version
; Set from refbox.cpp according to src/libs/core/version.h

(load* (resolve-file net.clp))
(load* (resolve-file machines.clp))
(load* (resolve-file robots.clp))
(load* (resolve-file orders.clp))
(load* (resolve-file simulation.clp))
(load* (resolve-file game.clp))
(load* (resolve-file setup.clp))
(load* (resolve-file production.clp))
(load* (resolve-file exploration.clp))
(load* (resolve-file challenges.clp))

(defrule rfid-input-learn-puck
  (declare (salience ?*PRIORITY_FIRST*))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  (machine (name ?m) (pose ?mpose-x ?mpose-y ?mpose-ori) (team ?team))
  (not (puck (id ?id)))
  =>
  (bind ?new-index 1)
  (do-for-all-facts ((?puck puck)) TRUE
    (bind ?new-index (max (+ ?puck:index 1) ?new-index))
  )
  (printout t "Learned new puck ID " ?id " for team " ?team
	    " (index " ?new-index ", state S0)" crlf)
  (assert (puck (index ?new-index) (id ?id) (state S0) (team ?team)))
)

(defrule rfid-input-cleanup
  (declare (salience ?*PRIORITY_CLEANUP*))
  ?f <- (rfid-input (machine ?m) (has-puck ?hp) (id ?id))
  =>
  (retract ?f)
  ;(if (debug 1) then
  ;  (printout t "Clearing unused RFID input (" ?m ", " ?hp ", " ?id ")" crlf))
)

(defrule config-timer-interval
  (confval (path "/llsfrb/clips/timer-interval") (type ?t) (value ?v))
  =>
  (bind ?*TIMER-INTERVAL* (/ ?v 1000.))
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

(defrule load-mongodb
  (init)
  (have-feature MongoDB)
  =>
  (printout t "Enabling MongoDB logging" crlf)
  (load* (resolve-file mongodb.clp))
)

(defrule reset-game
  ?gs <- (gamestate (state INIT) (prev-state ~INIT))
  =>
  (bind ?nc-id (create$))
  (bind ?nc-host (create$))
  (bind ?nc-port (create$))
  ; Remember network clients
  (do-for-all-facts ((?client network-client)) TRUE
    (bind ?nc-id   (create$ ?nc-id ?client:id))
    (bind ?nc-host (create$ ?nc-id ?client:host))
    (bind ?nc-port (create$ ?nc-id ?client:port))
  )
  ; reset the CLIPS environment
  (reset)
  (assert (init))
  ; restore network clients
  (foreach ?cid ?nc-id
    (assert (network-client (id ?cid)
			    (host (nth$ ?cid-index ?nc-host))
			    (port (nth$ ?cid-index ?nc-host))))
  )
)

(defrule retract-reset-game
  (declare (salience ?*PRIORITY_CLEANUP*))
  ?rf <- (reset-game)
  =>
  (retract ?rf)
)

(defrule finalize-print-points
  (finalize)
  =>
  (game-print-points)
  (printout t "===  Shutting down  ===" crlf)
)

; Sort pucks by ID, such that do-for-all-facts on the puck deftemplate
; iterates in a nice order, e.g. for net-send-PuckInfo
(defrule sort-pucks
  (declare (salience ?*PRIORITY_HIGH*))
  ?oa <- (puck (id ?id-a))
  ?ob <- (puck (id ?id-b&:(> ?id-a ?id-b)&:(< (fact-index ?oa) (fact-index ?ob))))
  =>
  (modify ?oa)
)

(defrule init-game
  ?gf <- (gamestate (state INIT))
  =>
  (modify ?gf (state WAIT_START) (last-time (now)))
)

