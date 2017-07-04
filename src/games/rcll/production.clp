
;---------------------------------------------------------------------------
;  production.clp - LLSF RefBox CLIPS production phase rules
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

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

(defrule prod-machine-down
  (declare (salience ?*PRIORITY_HIGHER*))
  (gamestate (phase PRODUCTION) (state RUNNING) (game-time ?gt))
  ?mf <- (machine (name ?name) (mtype ?mtype)
		  (state ?state&~DOWN) (proc-start ?proc-start)
		  (down-period $?dp&:(<= (nth$ 1 ?dp) ?gt)&:(>= (nth$ 2 ?dp) ?gt)))
  =>
  (bind ?down-time (- (nth$ 2 ?dp) (nth$ 1 ?dp)))
  (printout t "Machine " ?name " down for " ?down-time " sec" crlf)
  (if (eq ?state PROCESSING)
   then
    (modify ?mf (state DOWN) (desired-lights RED-ON) (prev-state ?state)
	    (proc-start (+ ?proc-start ?down-time)))
   else
    (modify ?mf (state DOWN) (prev-state ?state) (desired-lights RED-ON))
  )
)

(defrule prod-machine-up
  (declare (salience ?*PRIORITY_HIGHER*))
  (gamestate (phase PRODUCTION) (state RUNNING) (game-time ?gt))
  ?mf <- (machine (name ?name) (state DOWN) (prev-state ?prev-state&~DOWN)
		  (mps-state-deferred ?mps-state)
		  (down-period $?dp&:(<= (nth$ 2 ?dp) ?gt)))
  =>
  (printout t "Machine " ?name " is up again" crlf)
  (if (eq ?mps-state NONE)
   then (modify ?mf (state ?prev-state) (proc-state DOWN))
   else (modify ?mf (state ?prev-state) (proc-state DOWN)
		(mps-state ?mps-state) (mps-state-deferred NONE))
  )
)

(defrule prod-machine-prepare
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.PrepareMachine") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (client-type ?ct) (client-id ?cid))
  (network-peer (id ?cid) (group ?group))
  =>
  (retract ?pf)
  (bind ?mname (sym-cat (pb-field-value ?p "machine")))
  (bind ?team (sym-cat (pb-field-value ?p "team_color")))
  (if (and (eq ?ct PEER) (neq ?team ?group))
   then
    ; message received for a team over the wrong channel, deny
    (assert (attention-message (team ?group)
	      (text (str-cat "Invalid prepare for team " ?team " of team " ?group))))
   else
    (if (not (any-factp ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))))
     then
      (assert (attention-message (team ?team)
		(text (str-cat "Prepare received for invalid machine " ?mname))))
     else
      (printout t "Received prepare for " ?mname crlf)
      (do-for-fact ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))
        (if (eq ?m:state IDLE) then
	  (printout t ?mname " is IDLE, processing prepare" crlf)
	  (switch ?m:mtype
            (case BS then
	      (if (pb-has-field ?p "instruction_bs")
	       then
	        (bind ?prepmsg (pb-field-value ?p "instruction_bs"))
		(bind ?side (sym-cat (pb-field-value ?prepmsg "side")))
		(bind ?color (sym-cat (pb-field-value ?prepmsg "color")))
		(printout t "Prepared " ?mname " (side: " ?side ", color: " ?color ")" crlf)
	        (modify ?m (state PREPARED) (bs-side  ?side) (bs-color ?color))
               else
		(modify ?m (state BROKEN)(prev-state ?m:state)
			(broken-reason (str-cat "Prepare received for " ?mname " without data")))
	      )
            )
            (case DS then
	      (if (pb-has-field ?p "instruction_ds")
	       then
	        (bind ?prepmsg (pb-field-value ?p "instruction_ds"))
		(bind ?gate (pb-field-value ?prepmsg "gate"))
		(printout t "Prepared " ?mname " (gate: " ?gate ")" crlf)
	        (modify ?m (state PREPARED) (ds-gate ?gate))
               else
		(modify ?m (state BROKEN) (prev-state ?m:state)
			(broken-reason (str-cat "Prepare received for " ?mname " without data")))
	      )
            )
            (case SS then
	      (if (pb-has-field ?p "instruction_ss")
	       then
	        (bind ?prepmsg (pb-field-value ?p "instruction_ss"))
	        (bind ?task (pb-field-value ?prepmsg "task"))
		(bind ?operation (sym-cat (pb-field-value ?task "operation")))
		(bind ?slot (pb-field-value ?task "slot"))
                (bind ?slot-x (pb-field-value ?slot "x"))
                (bind ?slot-y (pb-field-value ?slot "y"))
                (bind ?slot-z (pb-field-value ?slot "z"))

                (if (eq ?operation RETRIEVE)
                 then
                  ; check if slot is filled
                  (if (any-factp ((?ss-slot machine-ss-filled)) (and (eq ?ss-slot:name ?mname)
                                                                     (eq (nth$ 1 ?ss-slot:slot) ?slot-x)
                                                                     (eq (nth$ 2 ?ss-slot:slot) ?slot-y)
                                                                     (eq (nth$ 3 ?ss-slot:slot) ?slot-z)
                                                                )
                      )
                   then
                    (printout t "Prepared " ?mname " (RETRIVE: (" ?slot-x ", " ?slot-y ", " ?slot-z ") )" crlf)
                    (modify ?m (state PREPARED) (ss-operation ?operation) (ss-slot ?slot-x ?slot-y ?slot-z))
                   else
		    (modify ?m (state BROKEN)(prev-state ?m:state) (broken-reason (str-cat "Prepare received for " ?mname " with RETRIVE (" ?slot-x ", " ?slot-y ", " ?slot-z ") but this is empty")))
                  )
                 else
                  (if (eq ?operation STORE)
                   then
		    (modify ?m (state BROKEN)(prev-state ?m:state) (broken-reason (str-cat "Prepare received for " ?mname " with STORE-operation")))
                   else
		    (modify ?m (state BROKEN)(prev-state ?m:state) (broken-reason (str-cat "Prepare received for " ?mname " with unknown operation")))
                  )
                )
               else
		(modify ?m (state BROKEN)(prev-state ?m:state)
			(broken-reason (str-cat "Prepare received for " ?mname " without data")))
	      )
            )
            (case RS then
	      (if (pb-has-field ?p "instruction_rs")
	       then
	        (bind ?prepmsg (pb-field-value ?p "instruction_rs"))
		(bind ?ring-color (sym-cat (pb-field-value ?prepmsg "ring_color")))
		(if (member$ ?ring-color ?m:rs-ring-colors)
		 then
		  (printout t "Prepared " ?mname " (ring color: " ?ring-color ")" crlf)
	          (modify ?m (state PREPARED) (rs-ring-color ?ring-color))
                 else
		  (modify ?m (state BROKEN) (prev-state ?m:state)
			  (broken-reason (str-cat "Prepare received for " ?mname
						  " for invalid ring color (" ?ring-color ")")))
                )
               else
		(modify ?m (state BROKEN) (prev-state ?m:state)
			(broken-reason (str-cat "Prepare received for " ?mname " without data")))
              )
            )
            (case CS then
	      (if (pb-has-field ?p "instruction_cs")
	       then
	        (bind ?prepmsg (pb-field-value ?p "instruction_cs"))
		(bind ?cs-op (sym-cat (pb-field-value ?prepmsg "operation")))
		(switch ?cs-op

		  (case RETRIEVE_CAP then
		    (if (not ?m:cs-retrieved)
		     then
 		      (printout t "Prepared " ?mname " (" ?cs-op ")" crlf)
	              (modify ?m (state PREPARED) (cs-operation ?cs-op))
                     else
		      (modify ?m (state BROKEN) (prev-state ?m:state)
			      (broken-reason (str-cat "Prepare received for " ?mname ": "
						      "cannot retrieve while already holding")))
                    )
                  )
		  (case MOUNT_CAP then
		    (if ?m:cs-retrieved
		     then
 		      (printout t "Prepared " ?mname " (" ?cs-op ")" crlf)
	              (modify ?m (state PREPARED) (cs-operation ?cs-op))
                     else
		      (modify ?m (state BROKEN) (prev-state ?m:state)
			      (broken-reason (str-cat "Prepare received for " ?mname
						      ": cannot mount without cap")))
                    )
		  )
                )
               else
		(modify ?m (state BROKEN) (prev-state ?m:state)
			(broken-reason (str-cat "Prepare received for " ?mname " without data")))
	      )
            )
          )
        )
      )
    )
  )
)

(defrule prod-machine-reset-by-team
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.ResetMachine") (ptr ?p)
		     (rcvd-from ?from-host ?from-port) (client-type ?ct) (client-id ?cid))
  (network-peer (id ?cid) (group ?group))
  =>
  (retract ?pf)
  (bind ?mname (sym-cat (pb-field-value ?p "machine")))
  (bind ?team (sym-cat (pb-field-value ?p "team_color")))
  (if (and (eq ?ct PEER) (neq ?team ?group))
   then
    ; message received for a team over the wrong channel, deny
    (assert (attention-message (team ?group)
	      (text (str-cat "Invalid reset for team " ?team " of team " ?group))))
   else
    (if (not (any-factp ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))))
     then
      (assert (attention-message (team ?team)
		(text (str-cat "Reset received for invalid machine " ?mname))))
     else
      (printout t "Received reset for " ?mname crlf)
      (do-for-fact ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))
	(modify ?m (state BROKEN) (prev-state ?m:state)
                   (broken-reason (str-cat "Machine " ?mname " resetted by the team " ?team)))
      )
    )
  )
)

; **** Machine state processing

(defrule prod-proc-state-idle
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state IDLE) (proc-state ~IDLE))
  =>
  (printout t "Machine " ?n " switching to IDLE state" crlf)
  (modify ?m (proc-state IDLE) (desired-lights GREEN-ON))
;  (mps-reset (str-cat ?n))
; TODO 2017
)

(defrule prod-proc-state-prepared-bs-and-ss
  "BS and SS station goes directly to processing"
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype BS|SS) (state PREPARED) (proc-state ~PREPARED))
  =>
  (printout t "Machine " ?n " switching directly to PROCESSING on prepare" crlf)
  (modify ?m (state PROCESSING) (proc-state PREPARED) (desired-lights GREEN-BLINK)
	  (prep-blink-start ?gt))
)

;(defrule prod-proc-state-prepared
;  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
;  ?m <- (machine (name ?n) (mtype ?t) (state PREPARED) (proc-state ~PREPARED))
;  =>
;  (printout t "Machine " ?n " of type " ?t " switching to PREPARED state" crlf)
;  (modify ?m (proc-state PREPARED) (desired-lights GREEN-BLINK)
;	  (prep-blink-start ?gt))
;)

(defrule prod-proc-state-prepared-stop-blinking
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state PREPARED|PROCESSING)
		 (actual-lights GREEN-BLINK) (desired-lights GREEN-BLINK)
		 (prep-blink-start ?bs&:(timeout-sec ?gt ?bs ?*PREPARED-BLINK-TIME*)))
  =>
  (printout t "Machine " ?n " in PREPARED state stopping blinking" crlf)
  (modify ?m (desired-lights GREEN-ON YELLOW-ON))
)

(defrule prod-proc-state-processing-bs-start
  "BS must be instructed to dispense base for processing"
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype BS) (state PROCESSING) (proc-state ~PROCESSING)
		 (bs-side ?side) (bs-color ?color))
  =>
  (printout t "Machine " ?n " dispensing " ?color " base on " ?side crlf)
  (printout t "Machine " ?n " push out " ?color " base " crlf)

  ; send to MPSes what to do
  (bind ?id (net-get-new-id))
  (bind ?s (net-create-bs-process ?m ?id ?color))

  (net-assert-mps-change ?id ?n ?gt PROCESS ?s)

  (modify ?m (proc-state PROCESSING) (desired-lights GREEN-ON YELLOW-ON)
             (processing-state PROCESS) (prev-processing-state NONE))
)

(defrule prod-proc-state-processing-ss-start
  "SS must be instructed to dispense a C0 for processing"
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype SS) (state PROCESSING) (proc-state ~PROCESSING)
		 (ss-operation ?operation) (ss-slot ?slot-x ?slot-y ?slot-z)
                 (team ?team))
  =>
  (switch ?operation
    (case STORE then
      (printout t "Machine " ?n " received operation " ?operation ", is not allowed in the game" crlf)
      ;TODO broken
    )
    (case RETRIEVE then
      (printout t "Machine " ?n " " ?operation " product from " ?slot-x " " ?slot-y " " ?slot-z crlf)

      (modify ?m (proc-state PROCESSING) (desired-lights GREEN-ON YELLOW-ON)
                 (processing-state PROCESS) (prev-processing-state NONE))

      (assert (machine-ss-manual-step (processing-state PROCESS) (time-since ?gt)))
      (assert (attention-message (team ?team) (text (str-cat "Team " ?team " need to fill SS with C0 from " ?slot-x " " ?slot-y " " ?slot-z))))
    )
    (default
      (printout t "Machine " ?n " received unknown operation " ?operation crlf)
      ;TODO break mps
    )
  )
)

(defrule prod-proc-state-processing-ds-start
  "DS ..."
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype DS) (state PREPARED) (proc-state ~PREPARED)
		 (ds-gate ?gate) (processing-state ~PROCESS))
  =>
  (printout t "Machine " ?n " processing to gate " ?gate crlf)

  ; send to MPSes what to do
  (bind ?id (net-get-new-id))
  (bind ?s (net-create-ds-process ?m ?id ?gate))

  (net-assert-mps-change ?id ?n ?gt PROCESS ?s)

  (modify ?m (proc-state PREPARED) (desired-lights GREEN-BLINK)
             (prep-blink-start ?gt) (ds-last-gate ?gate)
             (processing-state PROCESS) (prev-processing-state NONE))
)

(defrule prod-proc-state-processing-cs-start
  "CS ..."
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype CS) (state PREPARED) (proc-state ~PREPARED)
		 (cs-operation ?op))
  =>
  (printout t ?op " on machine " ?n ", wait for product" crlf)

  ; send to MPSes what to do
  (bind ?id (net-get-new-id))
  (bind ?s (net-create-mps-move-conveyor ?m ?id MIDDLE))

  (net-assert-mps-change ?id ?n ?gt WAIT-FOR-PRODUCT ?s)

  (modify ?m (proc-state PREPARED) (desired-lights GREEN-BLINK)
             (prep-blink-start ?gt) (waiting-for-product-since ?gt)
             (processing-state WAIT-FOR-PRODUCT) (prev-processing-state NONE)
  )
)

(defrule prod-proc-state-processing-rs-start
  "RS ..."
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype RS) (state PREPARED) (proc-state ~PREPARED)
		 (rs-ring-color ?ring-color) (rs-ring-colors $?ring-colors)
                 (bases-added ?ba) (bases-used ?bu))
  (ring-spec (color ?ring-color) (req-bases ?req-bases&:(>= (- ?ba ?bu) ?req-bases)))
  =>
  (printout t "Mounting ring " ?n " from slide " (member$ ?ring-color ?ring-colors) ", wait for product" crlf)

  ; send to MPSes what to do
  (bind ?id (net-get-new-id))
  (bind ?s (net-create-mps-move-conveyor ?m ?id MIDDLE))

  (net-assert-mps-change ?id ?n ?gt WAIT-FOR-PRODUCT ?s)

  (modify ?m (proc-state PREPARED) (desired-lights GREEN-BLINK)
             (prep-blink-start ?gt) (waiting-for-product-since ?gt)
             (processing-state WAIT-FOR-PRODUCT) (prev-processing-state NONE)
  )
)

(defrule prod-proc-state-processing-picked-up
  "base picked up from output"
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pb <- (pb-machine-reply (id ?id-final) (machine ?n))
  ?id-comm <- (mps-comm-msg (id ?id-final) (name ?n) (task WAIT-FOR-PICKUP))
  ?m <- (machine (name ?n) (state READY-AT-OUTPUT)
          (processing-state ?task-finished))
  =>
  (printout t "Machine " ?n " base piced up" crlf)
  ; TODO: Test gt vs ?id-comm time, time diff too big?

  (modify ?m (processing-state NONE) (prev-processing-state WAIT-FOR-PICKUP)
             (state IDLE) (proc-state READY-AT-OUTPUT))
  (retract ?id-comm ?pb)
)

(defrule prod-proc-state-processing-bs-intermedite
  "steps of the bs production cycle after first step"
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pb <- (pb-machine-reply (id ?id-final) (machine ?n))
  ?id-comm <- (mps-comm-msg (id ?id-final) (name ?n) (task ?task-finished))
  ?m <- (machine (name ?n) (mtype BS) (state PROCESSING)
          (processing-state ?task-finished) (bs-side ?side))
  =>
  (switch ?task-finished
    (case PROCESS then
      (printout t "Machine " ?n " move base to " ?side crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?
      (bind ?id (net-get-new-id))
      (bind ?s (net-create-mps-move-conveyor ?m ?id ?side))
    
      (net-assert-mps-change ?id ?n ?gt DRIVE-TO-OUT ?s)

      (modify ?m (processing-state DRIVE-TO-OUT) (prev-processing-state ?task-finished))
    )
    (case DRIVE-TO-OUT then
      (printout t "Machine " ?n " base ready for retreival at " ?side crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?

      (bind ?id (net-get-new-id))
      (bind ?s (net-create-mps-wait-for-pickup ?m ?id ?side))
    
      (net-assert-mps-change ?id ?n ?gt WAIT-FOR-PICKUP ?s)

      (modify ?m (processing-state WAIT-FOR-PICKUP) (prev-processing-state WAIT-FOR-PICKUP)
                 (state READY-AT-OUTPUT))
    )
    (default
      (printout error "Got mps-comm for machine " ?n " with unknown finished task " ?task-finished crlf)
    )
  )
  (retract ?id-comm ?pb)
)

(defrule prod-proc-state-processing-ds-intermedite
  "steps of the ds production cycle after first step"
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pb <- (pb-machine-reply (id ?id-final) (machine ?n))
  ?id-comm <- (mps-comm-msg (id ?id-final) (name ?n) (task ?task-finished))
  ?m <- (machine (name ?n) (mtype DS) (state PREPARED|PROCESSING)
          (processing-state ?task-finished) (ds-last-gate ?gate))
  =>
  (switch ?task-finished
    (case PROCESS then
      (printout t "Machine " ?n " wait for product" crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?
      (bind ?id (net-get-new-id))
      (bind ?s (net-create-mps-move-conveyor ?m ?id MIDDLE))
    
      (net-assert-mps-change ?id ?n ?gt WAIT-FOR-PRODUCT ?s)

      (modify ?m (processing-state WAIT-FOR-PRODUCT) (prev-processing-state ?task-finished)
                 (waiting-for-product-since ?gt)
      )
    )
    (case WAIT-FOR-PRODUCT then
      (printout t "Machine " ?n " received product at gate " ?gate crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?

      (modify ?m (processing-state NONE) (prev-processing-state WAIT-FOR-PRODUCT)
                 (state PROCESSED))
    )
    (default
      (printout error "Got mps-comm for machine " ?n " with unknown finished task " ?task-finished crlf)
    )
  )
  (retract ?id-comm ?pb)
)

(defrule prod-proc-state-processing-cs-intermedite
  "steps of the cs production cycle after first step"
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pb <- (pb-machine-reply (id ?id-final) (machine ?n))
  ?id-comm <- (mps-comm-msg (id ?id-final) (name ?n) (task ?task-finished))
  ?m <- (machine (name ?n) (mtype CS) (state PREPARED|PROCESSING)
          (processing-state ?task-finished) (cs-operation ?op))
  =>
  (switch ?task-finished
    (case WAIT-FOR-PRODUCT then
      (printout t "Machine " ?n " received product, process" crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?
      (bind ?id (net-get-new-id))
      (bind ?s (net-create-cs-process ?m ?id ?op))
    
      (net-assert-mps-change ?id ?n ?gt PROCESS ?s)

      (modify ?m (state PROCESSING) (desired-lights GREEN-ON YELLOW-ON)
                 (processing-state PROCESS) (prev-processing-state ?task-finished)
      )
    )
    (case PROCESS then
      (printout t "Machine " ?n " move base out" crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?
      (bind ?id (net-get-new-id))
      (bind ?s (net-create-mps-move-conveyor ?m ?id OUTPUT))
    
      (net-assert-mps-change ?id ?n ?gt DRIVE-TO-OUT ?s)

      (modify ?m (processing-state DRIVE-TO-OUT) (prev-processing-state ?task-finished))
    )
    (case DRIVE-TO-OUT then
      (printout t "Machine " ?n " base ready for retreival" crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?

      (bind ?id (net-get-new-id))
      (bind ?s (net-create-mps-wait-for-pickup ?m ?id OUTPUT))
    
      (net-assert-mps-change ?id ?n ?gt WAIT-FOR-PICKUP ?s)

      (modify ?m (processing-state WAIT-FOR-PICKUP) (prev-processing-state WAIT-FOR-PICKUP)
                 (state READY-AT-OUTPUT))
    )
    (default
      (printout error "Got mps-comm for machine " ?n " with unknown finished task " ?task-finished crlf)
    )
  )
  (retract ?id-comm ?pb)
)

(defrule prod-proc-state-processing-rs-intermedite
  "steps of the rs production cycle after first step"
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pb <- (pb-machine-reply (id ?id-final) (machine ?n))
  ?id-comm <- (mps-comm-msg (id ?id-final) (name ?n) (task ?task-finished))
  ?m <- (machine (name ?n) (mtype RS) (state PREPARED|PROCESSING)
          (processing-state ?task-finished) (rs-ring-color ?color) (bases-used ?bu))
  (ring-spec (color ?color) (req-bases ?req-bases))
  =>
  (switch ?task-finished
    (case WAIT-FOR-PRODUCT then
      (printout t "Machine " ?n " received product, process" crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?
      (bind ?id (net-get-new-id))
      (bind ?s (net-create-rs-process ?m ?id ?color))
    
      (net-assert-mps-change ?id ?n ?gt PROCESS ?s)

      (modify ?m (state PROCESSING) (desired-lights GREEN-ON YELLOW-ON)
                 (processing-state PROCESS) (prev-processing-state ?task-finished)
                 (bases-used (+ ?bu ?req-bases))
      )
    )
    (case PROCESS then
      (printout t "Machine " ?n " move base out" crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?
      (bind ?id (net-get-new-id))
      (bind ?s (net-create-mps-move-conveyor ?m ?id OUTPUT))
    
      (net-assert-mps-change ?id ?n ?gt DRIVE-TO-OUT ?s)

      (modify ?m (processing-state DRIVE-TO-OUT) (prev-processing-state ?task-finished))
    )
    (case DRIVE-TO-OUT then
      (printout t "Machine " ?n " base ready for retreival" crlf)
      ; TODO: Test gt vs ?id-comm time, time diff too big?

      (bind ?id (net-get-new-id))
      (bind ?s (net-create-mps-wait-for-pickup ?m ?id OUTPUT))
    
      (net-assert-mps-change ?id ?n ?gt WAIT-FOR-PICKUP ?s)

      (modify ?m (processing-state WAIT-FOR-PICKUP) (prev-processing-state WAIT-FOR-PICKUP)
                 (state READY-AT-OUTPUT))
    )
    (default
      (printout error "Got mps-comm for machine " ?n " with unknown finished task " ?task-finished crlf)
    )
  )
  (retract ?id-comm ?pb)
)

(defrule prod-proc-state-processing-ss-read-at-output-after-timeout
  "after the timeout the replenisher should have put the product onto the machine"
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype SS) (processing-state ?ps) (ss-slot ?slot-x ?slot-y ?slot-z))
  ?msm <- (machine-ss-manual-step (processing-state ?ps) (time-since ?ts&:(timeout-sec ?gt ?ts ?*SS-TIME-FOR-EACH-STEP*)))
  ?slot <- (machine-ss-filled (name ?n) (slot ?slot-x ?slot-y ?slot-z))
  =>
  (switch ?ps
    (case PROCESS then
      (printout t "Machine " ?n " base ready for retreival" crlf)

      (modify ?m (processing-state WAIT-FOR-PICKUP) (prev-processing-state ?ps)
                 (state READY-AT-OUTPUT))
      (assert (machine-ss-manual-step (processing-state WAIT-FOR-PICKUP) (time-since ?gt)))
    )
    (case WAIT-FOR-PICKUP then
      (printout t "Machine " ?n " base picked up" crlf)

      (modify ?m (processing-state NONE) (prev-processing-state ?ps)
                 (state IDLE))
      (retract ?slot)
    )
    (default
      (printout error "RefBox error in SS state machine" crlf)
    )
  )
  (retract ?msm)
)

(defrule prod-proc-state-processing-rs-insufficient-bases
  "Must check sufficient number of bases for RS"
  (declare (salience ?*PRIORITY_HIGHER*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype RS) (state PREPARED)
		 (rs-ring-color ?ring-color) (bases-added ?ba) (bases-used ?bu))
  (ring-spec (color ?ring-color)
	     (req-bases ?req-bases&:(> ?req-bases (- ?ba ?bu))))
  =>
  (modify ?m (state BROKEN) (proc-state PROCESSING)
	  (broken-reason (str-cat ?n ": insufficient bases ("
				  (- ?ba ?bu) " < " ?req-bases ")")))
)

(defrule prod-proc-state-processing-cs-mount-without-retrieve
  "Process on CS"
  (declare (salience ?*PRIORITY_HIGHER*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype CS) (state PROCESSING) (proc-state ~PROCESSING)
		 (cs-operation MOUNT_CAP) (cs-retrieved FALSE))
  =>
  ; TODO, can this be reached?
  (modify ?m (state BROKEN) (proc-state PROCESSING)
	  (broken-reason (str-cat ?n ": tried to mount without retrieving")))
)

(defrule prod-proc-state-processed-bs-ds-ss
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype BS|DS|SS) (state PROCESSED) (proc-state ~PROCESSED))
  =>
  (printout t "Machine " ?n " finished processing" crlf)
  (modify ?m (state IDLE) (proc-state PROCESSED))
)

(defrule prod-proc-state-processed-cs
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype CS) (state PROCESSED) (proc-state ~PROCESSED)
		 (cs-operation ?cs-op))
  =>
  (bind ?have-cap (eq ?cs-op RETRIEVE_CAP))
  (modify ?m (state IDLE) (proc-state PROCESSED) (cs-retrieved ?have-cap))
)

(defrule prod-proc-state-ready-at-output
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state READY-AT-OUTPUT) (proc-state ~READY-AT-OUTPUT))
  =>
  (printout t "Machine " ?n " finished processing, ready at output" crlf)
  (modify ?m (proc-state READY-AT-OUTPUT) (desired-lights YELLOW-ON))
)

(defrule prod-proc-state-retrieval-timeout
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state WAIT-IDLE)
		 (retrieved-at ?r&:(timeout-sec ?gt ?r ?*RETRIEVE-WAIT-IDLE-TIME*)))
  =>
  (printout t "retrieval timeout, going to IDLE" crlf)
  (modify ?m (state IDLE) (proc-state WAIT-IDLE))
)



(defrule prod-proc-state-broken
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state BROKEN) (proc-state ~BROKEN)
		 (team ?team) (broken-reason ?reason))
  =>
  (printout t "Machine " ?n " broken: " ?reason crlf)
  (assert (attention-message (team ?team) (text ?reason)))
  (modify ?m (proc-state BROKEN) (broken-since ?gt)
	  (desired-lights RED-BLINK YELLOW-BLINK))
)

(defrule prod-proc-state-broken-recover
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state BROKEN) (bases-added ?ba)
		 (broken-since ?bs&:(timeout-sec ?gt ?bs ?*BROKEN-DOWN-TIME*)))
  =>
  (printout t "Machine " ?n " recovered" crlf)
  (modify ?m (state IDLE) (prev-state BROKEN) (bases-used ?ba) (cs-retrieved FALSE))
)


; **** MPS state changes

(defrule prod-proc-mps-state-change
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?ms <- (machine-mps-state (name ?n) (state ?mps-state) (num-bases ?num-bases))
  ?m <- (machine (name ?n) (state ?state) (team ?team) (bases-added ?bases-added) (bases-used ?bases-used))
  (or (machine (name ?n) (mps-state ~?mps-state))
      (machine (name ?n) (bases-added ~?num-bases)))
  =>
  (printout t "Machine " ?n " MPS state " ?mps-state " (bases added: " ?num-bases ", state " ?state ")" crlf)
  (retract ?ms)
	(if (and (> ?num-bases ?bases-added)
					 (<= (- ?num-bases ?bases-used) ?*LOADED-WITH-MAX*))
	 then
	  (assert (points (game-time ?gt) (points ?*PRODUCTION-POINTS-ADDITIONAL-BASE*)
										(team ?team) (phase PRODUCTION)
										(reason (str-cat "Added additional base to " ?n))))
	)
  (if (eq ?state DOWN)
   then (modify ?m (mps-state-deferred ?mps-state) (bases-added ?num-bases))
   else (modify ?m (mps-state ?mps-state) (bases-added ?num-bases))
  )
)

(defrule prod-proc-mps-state-nochange
  "Cleanup machine states if no change occured"
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?ms <- (machine-mps-state (name ?n) (state ?mps-state))
  ?m <- (machine (name ?n) (mps-state ?mps-state))
  =>
  (retract ?ms)
)

; **** Mapping MPS to machine state reactions
(defrule prod-machine-reset
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))

  ?m <- (machine (name ?n) (state ?state&~IDLE) (mps-state RESET))
  =>
  (modify ?m (state IDLE) (prev-state IDLE) (proc-state IDLE) (desired-lights GREEN-ON)
	  (mps-state IDLE) (mps-state-deferred NONE) (broken-reason "")
    (ds-gate 0) (ds-last-gate 0) (cs-retrieved FALSE))
)
  

(defrule prod-machine-input-not-prepared
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state ?state&~PREPARED&~BROKEN&~DOWN&~PROCESSING) (mps-state AVAILABLE))
  =>
  (modify ?m (state BROKEN) (prev-state ?state)
	  (broken-reason (str-cat "Input to " ?n " while not prepared " ?state)))
)


(defrule prod-machine-loaded-with-too-many
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state ?state&~BROKEN&~DOWN) (bases-added ?ba)
		 (bases-used ?bu&:(> (- ?ba ?bu) ?*LOADED-WITH-MAX*)))
  =>
  (modify ?m (state BROKEN) (prev-state ?state)
	  (broken-reason (str-cat ?n ": too many additional bases loaded")))
)

(defrule prod-machine-input
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state PREPARED) (mps-state AVAILABLE))
  =>
  (modify ?m (state PROCESSING) (proc-start ?gt) (mps-state AVAILABLE-HANDLED))
)

(defrule prod-machine-proc-done
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state PROCESSING) (mps-state PROCESSED)
		 (proc-time ?pt) (proc-start ?pstart&:(timeout-sec ?gt ?pstart ?pt)))
  =>
  (modify ?m (state PROCESSED))
)

(defrule prod-machine-ready-at-output
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state PROCESSING|PROCESSED|WAIT-IDLE) (mps-state DELIVERED)
		 (proc-time ?pt) (proc-start ?pstart&:(timeout-sec ?gt ?pstart ?pt)))
  =>
  (modify ?m (state READY-AT-OUTPUT))
)

(defrule prod-machine-retrieved
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state READY-AT-OUTPUT) (mps-state RETRIEVED)
		 (proc-time ?pt) (proc-start ?pstart&:(timeout-sec ?gt ?pstart ?pt)))
  =>
  (modify ?m (state WAIT-IDLE) (retrieved-at ?gt) (desired-lights YELLOW-BLINK))
)


(defrule prod-pb-recv-SetMachineState
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.SetMachineState") (ptr ?p) (rcvd-via STREAM)
		       (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?mname (sym-cat (pb-field-value ?p "machine_name")))
  (bind ?state (sym-cat (pb-field-value ?p "state")))
  (printout t "Received state " ?state " for machine " ?mname crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?mname)
    (assert (machine-mps-state (name ?mname) (state ?state) (num-bases ?m:bases-added)))
  )
)

(defrule prod-pb-recv-MachineAddBase
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.MachineAddBase") (ptr ?p) (rcvd-via STREAM)
		       (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?mname (sym-cat (pb-field-value ?p "machine_name")))
  (printout t "Add base to machine " ?mname crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?mname)
    (assert (machine-mps-state (name ?mname) (state ?m:mps-state)
			       (num-bases (+ ?m:bases-added 1))))
  )
)
