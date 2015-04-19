
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

(defrule prod-machine-down
  (declare (salience ?*PRIORITY_HIGH*))
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
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (phase PRODUCTION) (state RUNNING) (game-time ?gt))
  ?mf <- (machine (name ?name) (state DOWN) (prev-state ?prev-state&~DOWN)
		  (down-period $?dp&:(<= (nth$ 2 ?dp) ?gt)))
  =>
  (printout t "Machine " ?name " is up again" crlf)
  (switch ?prev-state
    (case PROCESSING then (modify ?mf (state PROCESSING) (desired-lights GREEN-ON YELLOW-ON)))
    (case WAITING    then (modify ?mf (state WAITING)    (desired-lights YELLOW-ON)))
    (case INVALID    then (modify ?mf (state INVALID)    (desired-lights YELLOW-BLINK)))
    (case IDLE       then (modify ?mf (state IDLE)       (desired-lights GREEN-ON)))
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

; **** MPS state changes

(defrule prod-proc-mps-state-change
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?ms <- (machine-mps-state (name ?n) (state ?mps-state))
  ?m <- (machine (name ?n) (mps-state ?old-state&~?mps-state))
  =>
  (printout t "Machine " ?n " MPS state change to " ?mps-state crlf)
  (retract ?ms)
  (modify ?m (mps-state ?mps-state))
)

(defrule prod-proc-mps-state-nochange
  "Cleanup machine states if no change occured"
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?ms <- (machine-mps-state (name ?n) (state ?mps-state))
  ?m <- (machine (name ?n) (mps-state ?mps-state))
  =>
  (retract ?ms)
)


; **** Machine state processing

(defrule prod-proc-state-idle
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state IDLE) (proc-state ~IDLE))
  =>
  (printout t "Machine " ?n " switching to IDLE state" crlf)
  (modify ?m (proc-state IDLE) (desired-lights GREEN-ON))
)

(defrule prod-proc-state-prepared-bs
  "BS station goes directly to processing"
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype BS) (state PREPARED) (proc-state ~PREPARED))
  =>
  (printout t "Machine " ?n " switching directly to PROCESSING on prepare" crlf)
  (modify ?m (state PROCESSING) (proc-state PREPARED) (desired-lights GREEN-BLINK)
	  (prep-blink-start ?gt))
)

(defrule prod-proc-state-prepared
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype ?t) (state PREPARED) (proc-state ~PREPARED))
  =>
  (printout t "Machine " ?n " of type " ?t " switching to PREPARED state" crlf)
  (modify ?m (proc-state PREPARED) (desired-lights GREEN-BLINK)
	  (prep-blink-start ?gt))
)

(defrule prod-proc-state-prepared-stop-blinking
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state PREPARED|PROCESSING)
		 (actual-lights GREEN-BLINK) (desired-lights GREEN-BLINK)
		 (prep-blink-start ?bs&:(timeout-sec ?gt ?bs ?*PREPARED-BLINK-TIME*)))
  =>
  (printout t "Machine " ?n " in PREPARED state stopping blinking" crlf)
  (modify ?m (desired-lights GREEN-ON))
)

(defrule prod-proc-state-processing-bs
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype BS) (state PROCESSING) (proc-state ~PROCESSING)
		 (bs-side ?side) (bs-color ?color))
  =>
  (printout t "Machine " ?n " dispensing " ?color " base on " ?side crlf)
  (modify ?m (proc-state PROCESSING) (desired-lights GREEN-ON YELLOW-ON))
  ; TODO: (mps-instruct DISPENSE-BASE ?side ?color)
)

(defrule prod-proc-state-processing
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state PROCESSING) (proc-state ~PROCESSING))
  =>
  (printout t "Machine " ?n " starts processing" crlf)
  (modify ?m (proc-state PROCESSING) (desired-lights GREEN-ON YELLOW-ON))
  ; TODO: (mps-instruct PROCESS)
)


(defrule prod-proc-state-processed-ds
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype DS) (state PROCESSED) (proc-state ~PROCESSED))
  =>
  (printout t "Machine " ?n " finished processing, moving to output" crlf)
  (modify ?m (state IDLE) (proc-state PROCESSED))
  ; TODO: (mps-instruct PROCESSED)
)

(defrule prod-proc-state-processed
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state PROCESSED) (proc-state ~PROCESSED))
  =>
  (printout t "Machine " ?n " finished processing, moving to output" crlf)
  (modify ?m (proc-state PROCESSED))
  ; TODO: (mps-instruct PROCESSED)
)

(defrule prod-proc-state-ready-at-output
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state READY-AT-OUTPUT) (proc-state ~READY-AT-OUTPUT))
  =>
  (printout t "Machine " ?n " finished processing, ready at output" crlf)
  (modify ?m (proc-state READY-AT-OUTPUT))
)


(defrule prod-proc-state-broken
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state BROKEN) (proc-state ~BROKEN)
		 (team ?team) (broken-reason ?reason))
  =>
  (printout t "Machine " ?n " broken: " ?reason crlf)
  (assert (attention-message (team ?team) (text ?reason)))
  (modify ?m (proc-state BROKEN) (desired-lights RED-BLINK YELLOW-BLINK))
)


; **** Mapping MPS to machine state reactions

(defrule prod-machine-input-not-prepared
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state ?state&~PREPARED&~BROKEN) (mps-state AVAILABLE))
  =>
  (modify ?m (state BROKEN) (prev-state ?state)
	  (broken-reason (str-cat "Input to " ?n " while not prepared " ?state)))
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
  ?m <- (machine (name ?n) (state PROCESSED) (mps-state DELIVERED)
		 (proc-time ?pt) (proc-start ?pstart&:(timeout-sec ?gt ?pstart ?pt)))
  =>
  (modify ?m (state READY-AT-OUTPUT))
)


(defrule prod-machine-retrieved
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state READY-AT-OUTPUT) (mps-state RETRIEVED)
		 (proc-time ?pt) (proc-start ?pstart&:(timeout-sec ?gt ?pstart ?pt)))
  =>
  (modify ?m (state IDLE))
)


(defrule prod-pb-set-machine-state
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.SetMachineState") (ptr ?p) (rcvd-via STREAM)
		       (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?mname (sym-cat (pb-field-value ?p "machine_name")))
  (bind ?state (sym-cat (pb-field-value ?p "state")))
  (printout t "Received state " ?state " for machine " ?mname crlf)
  (assert (machine-mps-state (name ?mname) (state ?state)))
)
