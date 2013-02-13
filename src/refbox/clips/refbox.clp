
;---------------------------------------------------------------------------
;  refbox.clp - LLSF RefBox CLIPS main file
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate rfid-input
  (slot machine (type SYMBOL) (allowed-values M1 M2 M3 M4 M5 M6 M7 M8 M9 M10 D1 D2 D3 TEST R1 R2))
  (slot has-puck (type SYMBOL))
  (slot id (type INTEGER))
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


(defrule beacon-period
  (time $?now)
  ?f <- (beacon-signal (time $?t&:(timeout ?now ?t ?*BEACON_PERIOD*)) (seq ?seq))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (if (debug 3) then (printout t "Sending beacon" crlf))
  (bind ?beacon (pb-create "llsf_msgs.BeaconSignal"))
  (bind ?beacon-time (pb-field-value ?beacon "time"))
  (pb-set-field ?beacon-time "sec" (nth$ 1 ?now))
  (pb-set-field ?beacon-time "nsec" (* (nth$ 2 ?now) 1000))
  (pb-set-field ?beacon "time" ?beacon-time) ; destroys ?beacon-time!
  (pb-set-field ?beacon "seq" ?seq)
  (pb-set-field ?beacon "team_name" "LLSF")
  (pb-set-field ?beacon "peer_name" "RefBox")
  (pb-broadcast ?beacon)
  (pb-destroy ?beacon)
)


(defrule m-init "Initialize machines on startup"
  ?mf <- (machine (name ?m) (state REQINIT))
  =>
  (modify ?mf (state IDLE))
  (sps-set-signal (str-cat ?m) "GREEN" "ON")
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
  (sps-set-signal (str-cat ?m) "RED" "OFF")
)


(defrule m-shutdown "Shutdown machines at the end"
  (finalize)
  ?mf <- (machine (name ?m))
  =>
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
  (sps-set-signal (str-cat ?m) "RED" "OFF")
)


(defrule m-puck-placed
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype ?t&~T1) (puck-id 0))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (modify ?mf (puck-id ?id))
  (printout t "Got puck " ?id " under machine " ?m " (state: " ?ps ")" crlf)
)

(defrule m-puck-removed
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype ?t&~T1) (puck-id ?id&~0))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (modify ?mf (puck-id 0))
  (printout t "Removed puck " ?id " from machine " ?m crlf)
)


(defrule t1-proc-start
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype T1) (state IDLE))
  ?pf <- (puck (id ?id) (state S0))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?now))
  (sps-set-signal (str-cat ?m) "YELLOW" "ON")
)

(defrule t1-invalid-input
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype T1) (state IDLE) (puck-id 0))
  ?pf <- (puck (id ?id) (state ?ps&~S0))
  =>
  (modify ?mf (puck-id ?id) (state INVALID))
  (sps-set-signal (str-cat ?m) "YELLOW" "BLINK")
)

(defrule t1-proc-done
  (time $?now)
  ?mf <- (machine (name ?m) (mtype T1) (state PROCESSING) (puck-id ?id) (productions ?p)
		  (proc-start $?ps&:(timeout ?now ?ps ?*M1-PROC-TIME*)))
  ?pf <- (puck (id ?id) (state S0))
  =>
  (printout t "T1 production done @ " ?m ": " ?id " (S0 -> S1)" crlf)
  (modify ?mf (state IDLE) (productions (+ ?p 1)))
  (modify ?pf (state S1))
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
)

(defrule t1-removal
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype T1) (puck-id ?id&~0))
  ;?pf <- (puck (id ?id) (state S0))
  =>
  (modify ?mf (state IDLE) (puck-id 0))
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
)
