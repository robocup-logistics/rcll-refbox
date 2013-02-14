
;---------------------------------------------------------------------------
;  refbox.clp - LLSF RefBox CLIPS main file
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

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


(defrule t1-proc-start
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype T1) (state IDLE))
  ?pf <- (puck (id ?id) (state S0))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?now)
	  (proc-time (random ?*T1-PROC-TIME-MIN* ?*T1-PROC-TIME-MAX*)))
  (sps-set-signal (str-cat ?m) "YELLOW" "ON")
)

(defrule t1-invalid-input
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype T1) (state IDLE) (puck-id 0))
  ?pf <- (puck (id ?id) (state ?ps&~S0))
  =>
  (modify ?mf (puck-id ?id) (state INVALID))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "YELLOW" "BLINK")
)

(defrule t1-proc-done
  (time $?now)
  ?mf <- (machine (name ?m) (mtype T1) (state PROCESSING) (puck-id ?id) (productions ?p)
		  (proc-time ?pt) (proc-start $?ps&:(timeout ?now ?ps ?pt)))
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

(defrule t2-proc-start
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?pf <- (puck (id ?id) (state ?ps&S0|S1))
  ?mf <- (machine (name ?m) (mtype T2) (state IDLE|WAITING)
		  (loaded-with $?lw&:(not (member$ ?ps ?lw))))
  =>
  (if (= (length$ ?lw) 1) then
    ; last puck to add
    (bind ?proc-time (random ?*T2-PROC-TIME-MIN* ?*T2-PROC-TIME-MAX*))
   else
    ; intermediate puck to add
    (bind ?proc-time 2)
  )
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?now) (proc-time ?proc-time))
  (sps-set-signal (str-cat ?m) "GREEN" "ON")
  (sps-set-signal (str-cat ?m) "YELLOW" "ON")
)

(defrule t2-invalid-input
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  (or (and (puck (id ?id) (state ?ps&~S0&~S1))
	   ?mf <- (machine (name ?m) (mtype T2) (state IDLE|WAITING) (puck-id 0)))
      ; OR:
      (and (puck (id ?id) (state ?ps&S0|S1))
	   ?mf <- (machine (name ?m) (mtype T2) (state IDLE|WAITING) (puck-id 0)
			   (loaded-with $?lw&:(member$ ?ps ?lw))))
  )
  =>
  (modify ?mf (puck-id ?id) (state INVALID))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "YELLOW" "BLINK")
)

(defrule t2-proc-waiting
  (time $?now)
  ?mf <- (machine (name ?m) (mtype T2) (state PROCESSING) (puck-id ?id)
		  (loaded-with $?lw&:(< (length$ ?lw) 1))
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (printout t "T2 " ?ps " consumed @ " ?m ": " ?id crlf)
  (modify ?mf (state WAITING) (loaded-with (create$ ?lw ?ps)))
  (modify ?pf (state CONSUMED))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
)

(defrule t2-proc-done
  (time $?now)
  ?mf <- (machine (name ?m) (mtype T2) (state PROCESSING) (puck-id ?id)
		  (loaded-with $?lw&:(= (length$ ?lw) 1)) (productions ?p) (junk ?junk)
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (printout t "T2 production done @ " ?m ": " ?id " (" ?ps " -> S2, took " ?pt " sec)" crlf)
  (modify ?mf (state IDLE) (loaded-with) (productions (+ ?p 1)) (junk (+ ?junk (length$ ?lw))))
  (modify ?pf (state S2))
  (sps-set-signal (str-cat ?m) "GREEN" "ON")
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
)

(defrule t2-removal
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype T2) (loaded-with $?lw) (puck-id ?id&~0))
   ;?pf <- (puck (id ?id) (state S0))
  =>
  (if (> (length$ ?lw) 0) then
    (modify ?mf (state WAITING) (puck-id 0))
    (sps-set-signal (str-cat ?m) "GREEN" "OFF")
    (sps-set-signal (str-cat ?m) "YELLOW" "ON")
  else
    (modify ?mf (state IDLE) (puck-id 0))
    (sps-set-signal (str-cat ?m) "GREEN" "ON")
    (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
  )
)


(defrule t3-proc-start
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?pf <- (puck (id ?id) (state ?ps&S0|S1|S2))
  ?mf <- (machine (name ?m) (mtype T3) (state IDLE|WAITING)
		  (loaded-with $?lw&:(not (member$ ?ps ?lw))))
  =>
  (if (= (length$ ?lw) 2) then
    ; last puck to add
    (bind ?proc-time (random ?*T3-PROC-TIME-MIN* ?*T3-PROC-TIME-MAX*))
   else
    ; intermediate puck to add
    (bind ?proc-time 2)
  )
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?now) (proc-time ?proc-time))
  (sps-set-signal (str-cat ?m) "GREEN" "ON")
  (sps-set-signal (str-cat ?m) "YELLOW" "ON")
)

(defrule t3-invalid-input
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  (or (and (puck (id ?id) (state ?ps&~S0&~S1&~S2))
	   ?mf <- (machine (name ?m) (mtype T3) (state IDLE|WAITING) (puck-id 0)))
      ; OR:
      (and (puck (id ?id) (state ?ps&S0|S1|S2))
	   ?mf <- (machine (name ?m) (mtype T3) (state IDLE|WAITING) (puck-id 0)
			   (loaded-with $?lw&:(member$ ?ps ?lw))))
  )
  =>
  (modify ?mf (puck-id ?id) (state INVALID))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "YELLOW" "BLINK")
)

(defrule t3-proc-waiting
  (time $?now)
  ?mf <- (machine (name ?m) (mtype T3) (state PROCESSING) (puck-id ?id)
		  (loaded-with $?lw&:(< (length$ ?lw) 2))
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (printout t "T3 " ?ps " consumed @ " ?m ": " ?id crlf)
  (modify ?mf (state WAITING) (loaded-with (create$ ?lw ?ps)))
  (modify ?pf (state CONSUMED))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
)

(defrule t3-proc-done
  (time $?now)
  ?mf <- (machine (name ?m) (mtype T3) (state PROCESSING) (puck-id ?id)
		  (loaded-with $?lw&:(= (length$ ?lw) 2)) (productions ?p) (junk ?junk)
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (printout t "T3 production done @ " ?m ": " ?id " (" ?ps " -> S2, took " ?pt " sec)" crlf)
  (modify ?mf (state IDLE) (loaded-with) (productions (+ ?p 1)) (junk (+ ?junk (length$ ?lw))))
  (modify ?pf (state P1))
  (sps-set-signal (str-cat ?m) "GREEN" "ON")
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
)

(defrule t3-removal
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype T3) (loaded-with $?lw) (puck-id ?id&~0))
   ;?pf <- (puck (id ?id) (state S0))
  =>
  (if (> (length$ ?lw) 0) then
    (modify ?mf (state WAITING) (puck-id 0))
    (sps-set-signal (str-cat ?m) "GREEN" "OFF")
    (sps-set-signal (str-cat ?m) "YELLOW" "ON")
  else
    (modify ?mf (state IDLE) (puck-id 0))
    (sps-set-signal (str-cat ?m) "GREEN" "ON")
    (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
  )
)
