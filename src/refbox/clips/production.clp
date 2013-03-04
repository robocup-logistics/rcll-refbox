
;---------------------------------------------------------------------------
;  production.clp - LLSF RefBox CLIPS production phase rules
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

(defrule machine-enable-production
  ?gs <- (gamestate (phase PRODUCTION) (prev-phase ~PRODUCTION))
  =>
  (modify ?gs (prev-phase PRODUCTION) (game-time 0.0))
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (switch ?machine:state
      (case PROCESSING then (modify ?machine (desired-lights GREEN-ON YELLOW-ON)))
      (case WAITING    then (modify ?machine (desired-lights YELLOW-ON)))
      (case INVALID    then (modify ?machine (desired-lights YELLOW-BLINK)))
      (case DOWN       then (modify ?machine (desired-lights RED-ON)))
      (default (modify ?machine (desired-lights GREEN-ON)))
    )
  )
)

(defrule machine-proc-start
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  (machine (name ?m) (mtype ?mtype&~DELIVER&~TEST&~RECYCLE))
  (machine-spec (mtype ?mtype)  (inputs $?inputs)
		(proc-time-min ?pt-min) (proc-time-max ?pt-max))
  ?pf <- (puck (id ?id) (state ?ps&:(member$ ?ps ?inputs)))
  ?mf <- (machine (name ?m) (mtype ?mtype) (state IDLE|WAITING)
		  (loaded-with $?lw&:(not (any-puck-in-state ?ps ?lw))))
  (not (or (machine (name ?m2&~?m) (puck-id ?m2-pid&?id))
	   (machine (name ?m2&~?m) (loaded-with $?m2-lw&:(member$ ?id ?m2-lw)))))
  =>
  (if (= (length$ ?lw) (length$ ?inputs)) then
    ; last puck to add
    (bind ?proc-time (random ?pt-min ?pt-max))
   else
    ; intermediate puck to add
    (bind ?proc-time ?*INTERMEDIATE-PROC-TIME*)
  )
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?now) (proc-time ?proc-time)
	  (desired-lights GREEN-ON YELLOW-ON))
)


(defrule machine-invalid-input
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  (machine (name ?m) (mtype ?mtype&~DELIVER&~TEST&~RECYCLE))
  (machine-spec (mtype ?mtype)  (inputs $?inputs))
  (not (or (machine (name ?m2&~?m) (puck-id ?m2-pid&?id))
	   (machine (name ?m2&~?m) (loaded-with $?m2-lw&:(member$ ?id ?m2-lw)))))
  (or (and (puck (id ?id) (state ?ps&:(not (member$ ?ps ?inputs))))
	   ?mf <- (machine (name ?m) (state IDLE|WAITING) (puck-id 0)))
      ; OR:
      (and (puck (id ?id) (state ?ps&:(member$ ?ps ?inputs)))
	   ?mf <- (machine (name ?m) (state IDLE|WAITING) (puck-id 0)
			   (loaded-with $?lw&:(any-puck-in-state ?ps ?lw))))
  )
  =>
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights YELLOW-BLINK))
)

(defrule machine-invalid-input-junk
  "A puck was placed that was already placed at another machine"
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype ?mtype&~DELIVER&~TEST&~RECYCLE))
  ?pf <- (puck (id ?id))
  (or (machine (name ?m2&~?m) (puck-id ?m2-pid&?id))
      (machine (name ?m2&~?m) (loaded-with $?m2-lw&:(member$ ?id ?m2-lw))))
  =>
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights YELLOW-BLINK))
  (modify ?pf (state CONSUMED))
  (delayed-do-for-all-facts ((?machine machine)) (member$ ?id ?machine:loaded-with)
    (modify ?machine (loaded-with (delete-member$ ?machine:loaded-with ?id)))
  )
)

(defrule machine-proc-waiting
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  (machine (name ?m) (mtype ?mtype&~DELIVER&~TEST&~RECYCLE) (state PROCESSING))
  (machine-spec (mtype ?mtype)  (inputs $?inputs))
  ?mf <- (machine (name ?m) (mtype ?mtype) (puck-id ?id)
		  (loaded-with $?lw&:(< (+ (length$ ?lw) 1) (length$ ?inputs)))
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (printout t ?mtype ": " ?ps " consumed @ " ?m ": " ?id crlf)
  (modify ?mf (state WAITING) (loaded-with (create$ ?lw ?id)) (desired-lights YELLOW-ON))
  ;(modify ?pf (state CONSUMED))
)

(defrule machine-proc-done
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  ?gf <- (gamestate (points ?points))
  (machine (name ?m) (mtype ?mtype) (state PROCESSING))
  (machine-spec (mtype ?mtype&~DELIVER&~TEST&~RECYCLE)
		(inputs $?inputs) (output ?output) (points ?machine-points))
  ?mf <- (machine (name ?m) (mtype ?mtype) (puck-id ?id)
		  (loaded-with $?lw&:(= (+ (length$ ?lw) 1) (length$ ?inputs)))
		  (productions ?p) (junk ?junk)
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (printout t ?mtype " production done @ " ?m ": " ?id " (" ?ps
	    " -> " ?output ", took " ?pt " sec)" crlf)
  (modify ?mf (state IDLE) (loaded-with)  (desired-lights GREEN-ON)
	  (productions (+ ?p 1)) (junk (+ ?junk (length$ ?lw))))
  (modify ?gf (points (+ ?points ?machine-points)))
  (modify ?pf (state ?output))
  (foreach ?puck-id ?lw
    (do-for-fact ((?puck puck)) (= ?puck:id ?puck-id)
      (modify ?puck (state CONSUMED))
    )
  )
)

(defrule machine-puck-removal
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype ?mtype&~DELIVER&~TEST&~RECYCLE)
		  (loaded-with $?lw) (puck-id ?id&~0))
   ;?pf <- (puck (id ?id) (state S0))
  =>
  (if (> (length$ ?lw) 0) then
    (modify ?mf (state WAITING) (puck-id 0) (desired-lights YELLOW-ON))
  else
    (modify ?mf (state IDLE) (puck-id 0)  (desired-lights GREEN-ON))
  )
)

(defrule deliver-proc-start
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype DELIVER) (state IDLE))
  ?pf <- (puck (id ?id) (state ?ps))
  (order (active TRUE) (product ?product&:(eq ?product ?ps)))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?now)
	  (proc-time ?*DELIVER-PROC-TIME*) (desired-lights GREEN-ON YELLOW-ON))
)


(defrule deliver-invalid-input
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype DELIVER) (state IDLE) (puck-id 0))
  ?pf <- (puck (id ?id) (state ?ps))
  (not (order (active TRUE) (product ?product&:(eq ?product ?ps))))
  =>
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights YELLOW-BLINK))
)

(defrule deliver-proc-done
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  ?mf <- (machine (name ?m) (mtype DELIVER) (state PROCESSING) (puck-id ?id) (productions ?p)
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (printout t "Delivered " ?ps " @ " ?m ": " ?id " (" ?ps " -> CONSUMED)" crlf)
  (modify ?mf (state IDLE) (productions (+ ?p 1)) (desired-lights GREEN-ON YELLOW-ON RED-ON))
  (modify ?pf (state CONSUMED))
  (assert (product-delivered (time ?now) (product ?ps) (delivery-gate ?m))) 
)

(defrule deliver-removal
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype DELIVER) (puck-id ?id&~0))
  =>
  (modify ?mf (state IDLE) (puck-id 0) (desired-lights GREEN-ON))
)


(defrule recycle-proc-start
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (state IDLE))
  ?pf <- (puck (id ?id) (state CONSUMED))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?now)
	  (proc-time ?*RECYCLE-PROC-TIME*) (desired-lights GREEN-ON YELLOW-ON))
)

(defrule recycle-invalid-input
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (state IDLE) (puck-id 0))
  ?pf <- (puck (id ?id) (state ?ps&~CONSUMED))
  =>
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights YELLOW-BLINK))
)

(defrule recycle-proc-done
  (time $?now)
  (gamestate (state RUNNING) (phase PRODUCTION))
  ?gf <- (gamestate (points ?points))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (state PROCESSING) (puck-id ?id) (productions ?p)
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps&CONSUMED))
  =>
  (printout t "Recycling done @ " ?m ": " ?id " (" ?ps " -> S0)" crlf)
  (modify ?mf (state IDLE) (productions (+ ?p 1)) (desired-lights GREEN-ON))
  (modify ?pf (state S0))
  (modify ?gf (points (+ ?points ?*RECYCLE-POINTS*)))
)

(defrule recycle-removal
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (puck-id ?id&~0))
  =>
  (modify ?mf (state IDLE) (puck-id 0) (desired-lights GREEN-ON))
)


(defrule test-consumed
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state CONSUMED))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (desired-lights))
)

(defrule test-s0
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state S0))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (desired-lights YELLOW-ON))
)

(defrule test-s1
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state S1))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (desired-lights YELLOW-ON RED-ON))
)

(defrule test-s2
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state S2))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (desired-lights RED-ON))
)

(defrule test-p1
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state P1))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (desired-lights GREEN-BLINK))
)

(defrule test-p2
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state P2))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (desired-lights YELLOW-BLINK))
)

(defrule test-p3
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state P3))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (desired-lights RED-BLINK))
)

(defrule test-removal
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype TEST) (puck-id ?id&~0))
  =>
  (modify ?mf (state IDLE) (puck-id 0) (desired-lights GREEN-ON))
)
