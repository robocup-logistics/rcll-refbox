
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

(defrule production-start
  (declare (salience ?*PRIORITY_HIGH*))
  ?gs <- (gamestate (phase PRODUCTION) (prev-phase ~PRODUCTION))
  =>
  (modify ?gs (prev-phase PRODUCTION) (game-time 0.0))

  ; trigger machine info burst period
  (do-for-fact ((?sf signal)) (eq ?sf:type machine-info-bc)
    (modify ?sf (count 1) (time 0 0))
  )

  ; Set lights
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (modify ?machine (desired-lights GREEN-ON))
  )

  ;(assert (attention-message (text "Entering Production Phase")))
)

(defrule machine-down
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (phase PRODUCTION) (state RUNNING) (game-time ?gtime))
  ?mf <- (machine (name ?name) (mtype ?mtype) (puck-id ?puck-id)
		  (state ?state&~DOWN) (proc-start ?proc-start)
		  (down-period $?dp&:(<= (nth$ 1 ?dp) ?gtime)&:(>= (nth$ 2 ?dp) ?gtime)))
  =>
  (bind ?down-time (- (nth$ 2 ?dp) (nth$ 1 ?dp)))
  (printout t "Machine " ?name " down for " ?down-time " sec" crlf)
  (if (eq ?state PROCESSING)
   then
    (modify ?mf (state DOWN) (desired-lights RED-ON) (prev-state ?state)
	    (proc-start (+ ?proc-start ?down-time)))
   else
    (if (and (<> ?puck-id 0) (eq ?mtype DELIVER))
     then
      (modify ?mf (state DOWN) (prev-state ?state) (desired-lights RED-ON YELLOW-BLINK))
     else
      (modify ?mf (state DOWN) (prev-state ?state) (desired-lights RED-ON))
    )
  )
)

(defrule machine-up
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (phase PRODUCTION) (state RUNNING) (game-time ?gtime))
  ?mf <- (machine (name ?name) (state DOWN) (prev-state ?prev-state&~DOWN)
		  (down-period $?dp&:(<= (nth$ 2 ?dp) ?gtime)))
  =>
  (printout t "Machine " ?name " is up again" crlf)
  (switch ?prev-state
    (case PROCESSING then (modify ?mf (state PROCESSING) (desired-lights GREEN-ON YELLOW-ON)))
    (case WAITING    then (modify ?mf (state WAITING)    (desired-lights YELLOW-ON)))
    (case INVALID    then (modify ?mf (state INVALID)    (desired-lights YELLOW-BLINK)))
    (case IDLE       then (modify ?mf (state IDLE)       (desired-lights GREEN-ON)))
  )
)


(defrule machine-proc-start
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gtime))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  (machine (name ?m) (mtype ?mtype&~DELIVER&~RECYCLE) (team ?team))
  (machine-spec (mtype ?mtype)  (inputs $?inputs)
		(proc-time ?pt))
  (puck (id ?id) (state ?ps&:(member$ ?ps ?inputs)) (team ?team))
  ?mf <- (machine (name ?m) (mtype ?mtype) (state IDLE|WAITING)
		  (loaded-with $?lw&:(not (any-puck-in-state ?ps ?lw))))
  (not (or (machine (name ?m2&~?m) (puck-id ?m2-pid&?id))
	   (machine (name ?m2&~?m) (loaded-with $?m2-lw&:(member$ ?id ?m2-lw)))))
  =>
  (if (= (+ (length$ ?lw) 1) (length$ ?inputs)) then
    ; last puck to add
    (bind ?proc-time ?pt)
   else
    ; intermediate puck to add
    (bind ?proc-time ?*INTERMEDIATE-PROC-TIME*)
  )
  (printout t "Production begins at " ?m " (will take " ?proc-time " sec)" crlf)
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?gtime) (proc-time ?proc-time)
	  (desired-lights GREEN-ON YELLOW-ON))
)


(defrule machine-invalid-input
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  (machine (name ?m) (mtype ?mtype&~DELIVER&~RECYCLE) (team ?team))
  (machine-spec (mtype ?mtype)  (inputs $?inputs))
  (not (or (machine (name ?m2&~?m) (team ?team) (puck-id ?m2-pid&?id))
	   (machine (name ?m2&~?m) (team ?team) (loaded-with $?m2-lw&:(member$ ?id ?m2-lw)))))
  (or (and (puck (id ?id) (team ?team) (state ?ps&:(not (member$ ?ps ?inputs))))
	   ?mf <- (machine (name ?m) (state IDLE|WAITING) (puck-id 0) (team ?team)))
      ; OR:
      (and (puck (id ?id) (state ?ps&:(member$ ?ps ?inputs)) (team ?team))
	   ?mf <- (machine (name ?m) (state IDLE|WAITING) (puck-id 0) (team ?team)
			   (loaded-with $?lw&:(any-puck-in-state ?ps ?lw))))
  )
  =>
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights YELLOW-BLINK))
)

(defrule machine-wrong-team-input
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (state ~INVALID)
                  (mtype ?mtype&~DELIVER&~RECYCLE) (team ?machine-team))
  ?pf <- (puck (id ?id) (team ?puck-team&~?machine-team))
  =>
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights RED-BLINK YELLOW-BLINK))
  (modify ?pf (state FINISHED))
  (assert (points (game-time ?gt) (team ?machine-team) (phase PRODUCTION)
		  (points ?*PRODUCTION-WRONG-TEAM-MACHINE-POINTS*)
		  (reason (str-cat "Puck of team " ?puck-team " at "
				   ?m "|" ?mtype " of " ?machine-team))))
)

(defrule machine-invalid-input-junk
  "A puck was placed that was already placed at another machine"
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype ?mtype&~DELIVER&~RECYCLE))
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
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  (machine (name ?m) (mtype ?mtype&~DELIVER&~RECYCLE) (state PROCESSING))
  (machine-spec (mtype ?mtype)  (inputs $?inputs))
  ?mf <- (machine (name ?m) (mtype ?mtype) (team ?team) (puck-id ?id)
		  (loaded-with $?lw&:(< (+ (length$ ?lw) 1) (length$ ?inputs)))
		  (proc-time ?pt) (proc-start ?pstart&:(timeout-sec ?gt ?pstart ?pt)))
  ?pf <- (puck (id ?id) (team ?team) (state ?ps))
  =>
  (printout t ?mtype ": " ?ps " consumed @ " ?m ": " ?id " of " ?team crlf)
  (modify ?mf (state WAITING) (loaded-with (create$ ?lw ?id)) (desired-lights YELLOW-ON))
)

(defrule machine-proc-done
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  (machine (name ?m) (mtype ?mtype) (team ?team) (state PROCESSING))
  (machine-spec (mtype ?mtype&~DELIVER&~RECYCLE)
		(inputs $?inputs) (output ?output) (points ?machine-points))
  ?mf <- (machine (name ?m) (mtype ?mtype) (puck-id ?id)
		  (loaded-with $?lw&:(= (+ (length$ ?lw) 1) (length$ ?inputs)))
		  (productions ?p)
		  (proc-time ?pt) (proc-start ?pstart&:(timeout-sec ?gt ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps) (team ?team))
  =>
  (printout t ?mtype " production done @ " ?m ": " ?id " (" ?ps
	    " -> " ?output ", took " ?pt " sec, awarding " ?machine-points " points)" crlf)
  (modify ?mf (state IDLE) (loaded-with)  (desired-lights GREEN-ON)
	  (productions (+ ?p 1)))
  (assert (points (game-time ?gt) (team ?team) (points ?machine-points) (phase PRODUCTION)
		  (reason (str-cat ?mtype " production done at " ?m))))
  (modify ?pf (state ?output) (state-change-game-time ?gt))
  (foreach ?puck-id ?lw
    (do-for-fact ((?puck puck)) (= ?puck:id ?puck-id)
      (modify ?puck (state CONSUMED))
    )
  )
)

(defrule machine-puck-removal
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (state ?state) (mtype ?mtype&~DELIVER&~RECYCLE)
		  (loaded-with $?lw) (puck-id ?id&~0))
  =>
  (if (eq ?state DOWN)
   then
    (if (> (length$ ?lw) 0) then
      (modify ?mf (prev-state WAITING) (puck-id 0))
     else
      (modify ?mf (prev-state IDLE) (puck-id 0))
    )
   else
    (if (> (length$ ?lw) 0) then
      (modify ?mf (state WAITING) (puck-id 0) (desired-lights YELLOW-ON))
     else
      (modify ?mf (state IDLE) (puck-id 0)  (desired-lights GREEN-ON))
    )
  )
)

(defrule delivery-gate-down-period "Setup next delivery gate down period"
  (gamestate (phase PRODUCTION) (state RUNNING) (game-time ?gtime))
  (delivery-period (period $?p&:(>= ?gtime (nth$ 1 ?p))&:(<= ?gtime (nth$ 2 ?p)))
		   (delivery-gates $?dgates))
  ?mf <- (machine (mtype DELIVER) (name ?name&~:(member$ ?name ?dgates)) (state IDLE|INVALID))
  =>
  (modify ?mf (down-period ?p))
)

(defrule deliver-invalid-input
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype DELIVER) (state ?ms&IDLE|DOWN) (puck-id 0) (team ?team))
  ?pf <- (puck (id ?id) (state ?ps&~P1&~P2&~P3) (team ?team))
  ;(not (order (active TRUE) (product ?product&:(eq ?product ?ps))))
  =>
  (bind ?lights (create$ YELLOW-BLINK))
  (if (eq ?ms DOWN) then (bind ?lights (create$ RED-ON YELLOW-BLINK)))
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights ?lights))
  (modify ?pf (state FINISHED))

  (printout warn "Invalid delivery " ?ps " @ " ?m " (DOWN): " ?id " (" ?ps " -> FINISHED)" crlf)
  (assert (attention-message (team ?team)
			     (text (str-cat "Remove puck " ?id " from field @ " ?m))))
)

(defrule deliver-wrong-team-input
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype DELIVER) (team ?machine-team)
		  (state ?ms&IDLE|DOWN) (puck-id 0))
  ?pf <- (puck (id ?id) (team ?puck-team&~?machine-team))
  =>
  (bind ?lights (create$ RED-BLINK YELLOW-BLINK))
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights ?lights))
  (modify ?pf (state FINISHED))
  (assert (points (game-time ?gt) (team ?machine-team) (phase PRODUCTION)
		  (points ?*PRODUCTION-WRONG-TEAM-MACHINE-POINTS*)
		  (reason (str-cat "Delivery of puck of team " ?puck-team " at "
				   ?m " of " ?machine-team))))
)

(defrule deliver-proc-start
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gtime))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (team ?team) (mtype DELIVER) (state ?state) (down-period $?dp))
  (or
   (machine (name ?m) (state IDLE))
   ; OR:
   (machine (name ?m) (state DOWN)
	    (down-period $?dp&:(<= (- ?gtime (nth$ 1 ?dp)) ?*DELIVERY-GATE-GRACE-TIME*)))
  )
  (puck (id ?id) (state ?ps&~FINISHED) (team ?team))
  ;(order (active TRUE) (team ?team) (product ?product&:(eq ?product ?ps)))
  =>
  (if (eq ?state DOWN)
   then
    (bind ?dptmp (create$ (+ ?gtime ?*DELIVER-PROC-TIME*) (nth$ 2 ?dp)))
    (printout t "Grace time delivery @ " ?m crlf)
    (bind ?dp (create$ (+ ?gtime ?*DELIVER-PROC-TIME* ?*DELIVERY-GATE-GRACE-TIME*) (nth$ 2 ?dp)))
  )
  (modify ?mf (puck-id ?id) (state PROCESSING) (prev-state ?state) (proc-start ?gtime)
	  (down-period ?dp) (proc-time ?*DELIVER-PROC-TIME*) (desired-lights GREEN-ON YELLOW-ON))
)

(defrule deliver-proc-done
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gtime))
  ?mf <- (machine (name ?m) (mtype DELIVER) (state PROCESSING) (prev-state ?prev-state)
		  (team ?team) (puck-id ?id) (productions ?p)
		  (proc-time ?pt) (proc-start ?pstart&:(timeout-sec ?gtime ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps&~FINISHED) (team ?team)
	       (state-change-game-time ?production-time))
  =>
  (printout t "Delivered " ?ps " @ " ?m ": " ?id " (" ?ps " -> FINISHED)" crlf)
  (modify ?mf (state ?prev-state) (productions (+ ?p 1)) (desired-lights GREEN-ON YELLOW-ON RED-ON))
  (assert (attention-message (team ?team)
			     (text (str-cat "Remove puck " ?id " from field @ " ?m))))
  (modify ?pf (state FINISHED))
  (assert (product-delivered(game-time ?gtime) (product ?ps) (delivery-gate ?m)
			    (team ?team) (production-time ?production-time)))
)

(defrule deliver-down-machine
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype DELIVER) (state DOWN) (puck-id 0) (team ?team))
  ?pf <- (puck (id ?id) (state ?ps&P1|P2|P3) (team ?team))
  =>
  (bind ?lights (create$ RED-ON YELLOW-BLINK))
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights ?lights))
  (modify ?pf (state FINISHED))

  (printout warn "Invalid delivery " ?ps " @ " ?m " (DOWN): " ?id " (" ?ps " -> FINISHED)" crlf)
  (assert (attention-message (team ?team)
			     (text (str-cat "Remove puck " ?id " from field @ " ?m))))
)

(defrule deliver-removal
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (state ?state) (mtype DELIVER) (puck-id ?id&~0))
  =>
  (modify ?mf (puck-id 0) (state IDLE) (desired-lights GREEN-ON))
)


(defrule recycle-proc-start
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gtime))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (state IDLE) (team ?team))
  (puck (id ?id) (state CONSUMED) (team ?team))
  =>
  (modify ?mf (puck-id ?id) (state PROCESSING) (proc-start ?gtime)
	  (proc-time ?*RECYCLE-PROC-TIME*) (desired-lights GREEN-ON YELLOW-ON))
)

(defrule recycle-invalid-input
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (state IDLE) (puck-id 0) (team ?team))
  (puck (id ?id) (state ?ps&~CONSUMED) (team ?team))
  =>
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights YELLOW-BLINK))
)

(defrule recycle-wrong-team-input
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  (rfid-input (machine ?m) (has-puck TRUE) (id ?id&~0))
  ?mf <- (machine (name ?m) (team ?machine-team) (mtype RECYCLE) (state IDLE) (puck-id 0))
  ?pf <- (puck (id ?id) (team ?puck-team&~?machine-team))
  =>
  (modify ?mf (puck-id ?id) (state INVALID) (desired-lights RED-BLINK YELLOW-BLINK))
  (modify ?pf (state FINISHED))
  (assert (points (game-time ?gt) (team ?machine-team) (phase PRODUCTION)
		  (points ?*PRODUCTION-WRONG-TEAM-MACHINE-POINTS*)
		  (reason (str-cat "Recycling of puck of team " ?puck-team " at machine "
				   ?m " of " ?machine-team))))
)

(defrule recycle-proc-done
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (state PROCESSING)
		  (team ?team) (puck-id ?id) (productions ?p)
		  (proc-time ?pt) (proc-start ?pstart&:(timeout-sec ?gt ?pstart ?pt)))
  ?pf <- (puck (id ?id) (state ?ps&CONSUMED) (team ?team))
  =>
  (printout t "Recycling done by " ?team " @ " ?m ": " ?id " (" ?ps " -> S0). "
	    "Awarding " ?*RECYCLE-POINTS* " points." crlf)
  (modify ?mf (state IDLE) (productions (+ ?p 1)) (desired-lights GREEN-ON))
  (modify ?pf (state S0))
  (assert (points (game-time ?gt) (team ?team) (points ?*RECYCLE-POINTS*)  (phase PRODUCTION)
		  (reason (str-cat "Recycling done at " ?m))))
)

(defrule recycle-removal
  (gamestate (state RUNNING) (phase PRODUCTION))
  (rfid-input (machine ?m) (has-puck FALSE))
  ?mf <- (machine (name ?m) (mtype RECYCLE) (puck-id ?id&~0))
  =>
  (modify ?mf (state IDLE) (puck-id 0) (desired-lights GREEN-ON))
)


(defrule prod-net-recv-PlacePuckUnderMachine
  ?pf <- (protobuf-msg (type "llsf_msgs.PlacePuckUnderMachine") (ptr ?p) (rcvd-via STREAM))
  (gamestate (phase PRODUCTION) (state RUNNING))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (bind ?id (pb-field-value ?p "puck_id"))
  ; retract all existing rfid-input facts for this puck, can happen if SPS
  ; is enabled and then a network message is received
  (delayed-do-for-all-facts ((?input rfid-input)) (= ?input:id ?id)
    (retract ?input)
  )
  (bind ?machine (sym-cat (pb-field-value ?p "machine_name")))
  (printout t "Placing puck " ?id " under machine " ?machine crlf)
  (assert (rfid-input (machine ?machine) (has-puck TRUE) (id ?id)))
)

(defrule prod-net-recv-PlacePuckUnderMachine-not-running
  ?pf <- (protobuf-msg (type "llsf_msgs.PlacePuckUnderMachine") (ptr ?p) (rcvd-via STREAM))
  (gamestate (phase PRODUCTION) (state ~RUNNING))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (bind ?id (pb-field-value ?p "puck_id"))
  (bind ?machine (sym-cat (pb-field-value ?p "machine_name")))
  (printout warn "Cannot place puck " ?id " under machine " ?machine " when not RUNNING" crlf)
)

(defrule prod-net-recv-PlacePuckUnderMachine-illegal
  (declare (salience ?*PRIORITY_HIGH*))
  ?pf <- (protobuf-msg (type "llsf_msgs.PlacePuckUnderMachine") (ptr ?p)
		       (rcvd-via BROADCAST) (rcvd-from ?host ?port))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (printout warn "Illegal PlacePuckUnderMachine message received from host " ?host crlf)
)

(defrule prod-net-recv-PlacePuckUnderMachine-out-of-phase
  ?pf <- (protobuf-msg (type "llsf_msgs.PlacePuckUnderMachine") (ptr ?p)
		       (rcvd-via STREAM) (rcvd-from ?host ?port))
  (gamestate (phase ~PRODUCTION))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (printout warn "Received PlacePuckUnderMachine while not in PRODUCTION from host " ?host crlf)
)

(defrule prod-net-recv-LoadPuckInMachine
  ?pf <- (protobuf-msg (type "llsf_msgs.LoadPuckInMachine") (ptr ?p)
		       (rcvd-via STREAM) (rcvd-from ?host ?port))
  (gamestate (phase PRODUCTION))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (do-for-fact ((?machine machine))
	       (eq ?machine:name (sym-cat (pb-field-value ?p "machine_name")))
    (bind ?puck-id (pb-field-value ?p "puck_id"))
    (if (not (member$ ?puck-id ?machine:loaded-with))
      then
       (bind ?new-loaded-with (create$ ?machine:loaded-with ?puck-id))
       (assert (machine-update-loaded-with ?machine:name ?new-loaded-with))
    )
  )
)

(defrule prod-net-recv-LoadPuckInMachine-illegal
  (declare (salience ?*PRIORITY_HIGH*))
  ?pf <- (protobuf-msg (type "llsf_msgs.LoadPuckInMachine") (ptr ?p)
		       (rcvd-via BROADCAST) (rcvd-from ?host ?port))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (printout warn "Illegal LoadPuckInMachine message received from host " ?host crlf)
)

(defrule prod-net-recv-LoadPuckInMachine-out-of-phase
  ?pf <- (protobuf-msg (type "llsf_msgs.LoadPuckInMachine") (ptr ?p)
		       (rcvd-via STREAM) (rcvd-from ?host ?port))
  (gamestate (phase ~PRODUCTION))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (printout warn "Received LoadPuckInMachine while not in PRODUCTION from host " ?host crlf)
)

(defrule prod-net-recv-RemovePuckFromMachine
  ?pf <- (protobuf-msg (type "llsf_msgs.RemovePuckFromMachine") (ptr ?p))
  (gamestate (phase PRODUCTION))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  ;(printout t "Removing from Machine " (pb-field-value ?p "machine_name") crlf) 
  (do-for-fact ((?machine machine))
	       (eq ?machine:name (sym-cat (pb-field-value ?p "machine_name")))
    (bind ?puck-id (pb-field-value ?p "puck_id"))
    (if (= ?machine:puck-id ?puck-id)
      then
        ; retract all existing rfid-input facts for this puck, can happen if SPS
        ; is enabled and then a network message is received
        (delayed-do-for-all-facts ((?input rfid-input)) (= ?input:id ?puck-id)
          (retract ?input)
	)
	(printout t "Removing puck " ?puck-id " from machine " ?machine:name crlf)
        (assert (rfid-input (machine (sym-cat (pb-field-value ?p "machine_name")))
			    (has-puck FALSE)))
      else
      (if (member$ ?puck-id ?machine:loaded-with)
      then
        (bind ?new-loaded-with (delete-member$ ?machine:loaded-with ?puck-id))
	(assert (machine-update-loaded-with ?machine:name ?new-loaded-with))
	(do-for-fact ((?puck puck)) (eq ?puck:id ?puck-id)
          (printout t "Change puck " ?puck-id " from " ?puck:state " to CONSUMED"
		    " on user instructed removal" crlf)
	  (modify ?puck (state CONSUMED))
        )
      )
    )
  )
)

(defrule prod-net-recv-RemovePuckFromMachine-illegal
  ?pf <- (protobuf-msg (type "llsf_msgs.RemovePuckFromMachine") (ptr ?p)
		       (rcvd-via BROADCAST) (rcvd-from ?host ?port))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (printout warn "Illegal RemovePuckFromMachine message received from host " ?host crlf)
)

(defrule prod-net-recv-RemovePuckFromMachine-out-of-phase
  ?pf <- (protobuf-msg (type "llsf_msgs.RemovePuckFromMachine") (ptr ?p)
		       (rcvd-via BROADCAST) (rcvd-from ?host ?port))
  (gamestate (phase ~PRODUCTION))
  =>
  (retract ?pf) ; message will be destroyed after rule completes
  (printout warn "Received RemovePuckFromMachine while not in PRODUCTION from host " ?host crlf)
)


(defrule machine-update-loaded-with-no-inputs
  (declare (salience ?*PRIORITY_HIGH*))
  ?uf <- (machine-update-loaded-with ?m $?new-lw)
  (machine (name ?m) (mtype ?mtype))
  (machine-spec (mtype ?mtype) (inputs $?inputs&:(= (length$ ?inputs) 0)))
  =>
  (retract ?uf)
  (printout t "Ignoring load update " ?new-lw " for " ?m "|" ?mtype " w/o inputs" crlf)
)

(defrule machine-update-loaded-with-full
  (declare (salience ?*PRIORITY_HIGH*))
  ?uf <- (machine-update-loaded-with ?m $?new-lw)
  (machine (name ?m) (mtype ?mtype))
  (machine-spec (mtype ?mtype) (inputs $?inputs&:(< (length$ ?inputs) (length$ ?new-lw))))
  =>
  (retract ?uf)
  (printout t "Ignoring load update " ?new-lw " for " ?m "|" ?mtype " being already full" crlf)
)

(defrule machine-update-loaded-with-puck
  (declare (salience ?*PRIORITY_HIGH*))
  ?uf <- (machine-update-loaded-with ?m $?new-lw)
  (machine (name ?m) (mtype ?mtype) (puck-id ?puck-id&~0))
  (machine-spec (mtype ?mtype) (inputs $?inputs&:(= (length$ ?inputs) (length$ ?new-lw))))
  =>
  (retract ?uf)
  (printout t "Load update conflict " ?new-lw " for " ?m "|" ?mtype
	    " with puck under RFID" crlf)
)

(defrule machine-update-loaded-with
  (declare (salience ?*PRIORITY_HIGH*))
  ?gf <- (gamestate (phase PRODUCTION) (game-time ?gt))
  ?uf <- (machine-update-loaded-with ?m $?new-lw)
  ?mf <- (machine (name ?m) (mtype ?mtype) (loaded-with $?old-lw) (productions ?p))
  (machine-spec (mtype ?mtype) (inputs $?inputs&:(> (length$ ?inputs) 0))
		(output ?output) (points ?machine-points))
  =>
  (retract ?uf)
  (printout t "Updating " ?m "|" ?mtype " load from " ?old-lw " to " ?new-lw crlf)
  (bind ?new-lw-size (length$ ?new-lw))
  
  (if (= ?new-lw-size (length$ ?inputs))
   then ; production at this machine is complete
    (modify ?mf (state IDLE) (loaded-with)  (desired-lights GREEN-ON)
  	    (productions (+ ?p 1)))
    (assert (points (game-time ?gt) (points ?machine-points) (phase PRODUCTION)
		    (reason (str-cat "Production step at " ?m "|" ?mtype))))
    ;(modify ?pf (state ?output))
    (delayed-do-for-all-facts ((?puck puck)) (member$ ?puck:id ?new-lw)
      (if (member$ ?puck:id ?old-lw)
        then (modify ?puck (state CONSUMED))
        else (modify ?puck (state ?output))
      )
    )
  )
  (if (= ?new-lw-size 0)
   then ; all pucks have been removed
    (modify ?mf (state IDLE) (loaded-with)  (desired-lights GREEN-ON))
  )
  (if (> ?new-lw-size 0)
   then ; partial input, more pucks required to complete
    (modify ?mf (state WAITING) (loaded-with ?new-lw) (desired-lights YELLOW-ON))
  )
) 


