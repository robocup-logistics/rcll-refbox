
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

(defrule machine-lights
  ?mf <- (machine (name ?m) (actual-lights $?al) (desired-lights $?dl&:(neq ?al ?dl)))
  =>
  (printout t ?m " actual lights: " ?al "  desired: " ?dl crlf)
  (modify ?mf (actual-lights ?dl))
  (foreach ?c (create$ GREEN YELLOW RED)
    (sps-set-signal (str-cat ?m) (str-cat ?c)
		    (if (member$ ?c ?dl) then "ON" else "OFF"))
  )
)

(defrule machine-proc-start
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  (machine (name ?m) (mtype ?mtype))
  (machine-spec (mtype ?mtype)  (inputs $?inputs)
		(proc-time-min ?pt-min) (proc-time-max ?pt-max))
  ?pf <- (puck (id ?id) (state ?ps&:(member$ ?ps ?inputs)))
  ?mf <- (machine (name ?m) (mtype ?mtype) (state IDLE|WAITING)
		  (loaded-with $?lw&:(not (member$ ?ps ?lw))))
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
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  (machine (name ?m) (mtype ?mtype))
  (machine-spec (mtype ?mtype)  (inputs $?inputs))
  (or (and (puck (id ?id) (state ?ps&:(not (member$ ?ps ?inputs))))
	   ?mf <- (machine (name ?m) (state IDLE|WAITING) (puck-id 0)))
      ; OR:
      (and (puck (id ?id) (state ?ps&:(member$ ?ps ?inputs)))
	   ?mf <- (machine (name ?m) (state IDLE|WAITING) (puck-id 0)
			   (loaded-with $?lw&:(member$ ?ps ?lw))))
  )
  =>
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights YELLOW-BLINK))
  ;(sps-set-signal (str-cat ?m) "GREEN" "OFF")
  ;(sps-set-signal (str-cat ?m) "YELLOW" "BLINK")
)

(defrule machine-proc-waiting
  (time $?now)
  (machine (name ?m) (mtype ?mtype) (state PROCESSING))
  (machine-spec (mtype ?mtype)  (inputs $?inputs))
  ?mf <- (machine (name ?m) (mtype ?mtype) (puck-id ?id)
		  (loaded-with $?lw&:(< (+ (length$ ?lw) 1) (length$ ?inputs)))
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps))
  =>
  (printout t ?mtype ": " ?ps " consumed @ " ?m ": " ?id crlf)
  (modify ?mf (state WAITING) (loaded-with (create$ ?lw ?ps)) (desired-lights YELLOW-ON))
  (modify ?pf (state CONSUMED))
  ;(sps-set-signal (str-cat ?m) "GREEN" "OFF")
)

(defrule machine-proc-done
  (time $?now)
  (machine (name ?m) (mtype ?mtype) (state PROCESSING))
  (machine-spec (mtype ?mtype) (inputs $?inputs) (output ?output))
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
  (modify ?pf (state ?output))
  ;(sps-set-signal (str-cat ?m) "GREEN" "ON")
  ;(sps-set-signal (str-cat ?m) "YELLOW" "OFF")
)

(defrule machine-puck-removal
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (loaded-with $?lw) (puck-id ?id&~0))
   ;?pf <- (puck (id ?id) (state S0))
  =>
  (if (> (length$ ?lw) 0) then
    (modify ?mf (state WAITING) (puck-id 0) (desired-lights YELLOW-ON))
    ;(sps-set-signal (str-cat ?m) "GREEN" "OFF")
    ;(sps-set-signal (str-cat ?m) "YELLOW" "ON")
  else
    (modify ?mf (state IDLE) (puck-id 0)  (desired-lights GREEN-ON))
    ;(sps-set-signal (str-cat ?m) "GREEN" "ON")
    ;(sps-set-signal (str-cat ?m) "YELLOW" "OFF")
  )
)


(defrule recycle-proc-start
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (state IDLE))
  ?pf <- (puck (id ?id) (state JUNK|CONSUMED))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?now)
	  (proc-time ?*RECYCLE-PROC-TIME*))
  (sps-set-signal (str-cat ?m) "YELLOW" "ON")
)

(defrule recycle-invalid-input
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (state IDLE) (puck-id 0))
  ?pf <- (puck (id ?id) (state ?ps&~JUNK&~CONSUMED))
  =>
  (modify ?mf (puck-id ?id) (state INVALID))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "YELLOW" "BLINK")
)

(defrule recycle-proc-done
  (time $?now)
  ?mf <- (machine (name ?m) (mtype RECYCLE) (state PROCESSING) (puck-id ?id) (productions ?p)
		  (proc-time ?pt) (proc-start $?pstart&:(timeout ?now ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps&JUNK|CONSUMED))
  =>
  (printout t "Recycling done @ " ?m ": " ?id " (" ?ps " -> S0)" crlf)
  (modify ?mf (state IDLE) (productions (+ ?p 1)))
  (modify ?pf (state S0))
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
)

(defrule recycle-removal
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (puck-id ?id&~0))
  =>
  (modify ?mf (state IDLE) (puck-id 0))
  (sps-set-signal (str-cat ?m) "GREEN" "ON")
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
)


(defrule test-consumed-junk
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state JUNK|CONSUMED))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
)

(defrule test-s0
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state S0))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "YELLOW" "ON")
)

(defrule test-s1
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state S1))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "YELLOW" "ON")
  (sps-set-signal (str-cat ?m) "RED" "ON")
)

(defrule test-s2
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state S2))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "RED" "ON")
)

(defrule test-p1
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state P1))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING))
  (sps-set-signal (str-cat ?m) "GREEN" "BLINK")
)

(defrule test-p2
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state P2))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "YELLOW" "BLINK")
)

(defrule test-p3
  (time $?now)
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype TEST) (state IDLE))
  ?pf <- (puck (id ?id) (state P3))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING))
  (sps-set-signal (str-cat ?m) "GREEN" "OFF")
  (sps-set-signal (str-cat ?m) "RED" "BLINK")
)

(defrule test-removal
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype TEST) (puck-id ?id&~0))
  =>
  (modify ?mf (state IDLE) (puck-id 0))
  (sps-set-signal (str-cat ?m) "GREEN" "ON")
  (sps-set-signal (str-cat ?m) "YELLOW" "OFF")
  (sps-set-signal (str-cat ?m) "RED" "OFF")
)
