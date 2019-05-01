
;---------------------------------------------------------------------------
;  refbox.clp - LLSF RefBox CLIPS main file
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

; LLSF RefBox Version
; Set from refbox.cpp according to src/libs/core/version.h

(load* (resolve-file net.clp))
(if (config-get-bool "/llsfrb/simulation/enable")
  then (printout t "Enabling simulation" crlf) (load* (resolve-file simulation.clp)))
(load* (resolve-file machines.clp))
(load* (resolve-file workpieces.clp))
(load* (resolve-file robots.clp))
(load* (resolve-file orders.clp))
(load* (resolve-file game.clp))
(load* (resolve-file setup.clp))
(load* (resolve-file production.clp))
(load* (resolve-file exploration.clp))
(load* (resolve-file machine-lights.clp))
;(load* (resolve-file challenges.clp))

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
(defrule simulation-disabled
  (init)
  (confval (path "/llsfrb/simulation/enable") (type BOOL) (value false))
  =>
  (assert (sim-time (enabled false)))
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

(defrule init-game
  ?gf <- (gamestate (state INIT))
  =>
  (modify ?gf (state WAIT_START) (last-time (now)))
)

