
;---------------------------------------------------------------------------
;  orders.clp - LLSF RefBox CLIPS order processing
;
;  Created: Sun Feb 24 19:40:32 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2019  Mostafa Gomaa  [mostafa.gomaa@rwth-aachen.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction order-q-del-index (?team)
  (if (eq ?team CYAN) then (return 1) else (return 2))
)

(deffunction order-q-del-other-index (?team)
	(if (eq ?team CYAN)
	 then
		(return (order-q-del-index MAGENTA))
	 else
		(return (order-q-del-index CYAN))
	)
)

(deffunction order-q-del-team (?q-del ?team)
	(return (nth$ (order-q-del-index ?team) ?q-del))
)

(defrule activate-order
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?of <- (order (id ?id) (active FALSE) (activate-at ?at&:(>= ?gt ?at))
		(complexity ?c) (quantity-requested ?q) (delivery-period $?period))
  ?sf <- (signal (type order-info))
  =>
  (modify ?of (active TRUE))
  (modify ?sf (count 1) (time 0 0))
  (assert (attention-message (text (str-cat "Order " ?id ": " ?q " x " ?c " from "
					    (time-sec-format (nth$ 1 ?period)) " to "
					    (time-sec-format (nth$ 2 ?period))))
			     (time 10)))
)

; Sort orders by ID, such that do-for-all-facts on the orders deftemplate
; iterates in a nice order, e.g. for net-send-OrderInstruction
(defrule sort-orders
  (declare (salience ?*PRIORITY_HIGH*))
  ?oa <- (order (id ?id-a))
  ?ob <- (order (id ?id-b&:(> ?id-a ?id-b)&:(< (fact-index ?oa) (fact-index ?ob))))
  =>
  (modify ?oa)
)

(defrule order-recv-SetOrderDelivered
  (gamestate (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.SetOrderDelivered") (ptr ?p) (rcvd-via STREAM))
  =>
  (bind ?team (pb-field-value ?p "team_color"))
  (bind ?order (pb-field-value ?p "order_id"))
  (bind ?wp-id (workpiece-simulate-tracking ?order ?team ?gt))
  (assert (product-processed (game-time ?gt) (team ?team) (order ?order) (confirmed TRUE)
                             (workpiece ?wp-id) (mtype DS) (scored FALSE)))
  (printout t "Delivery by team " ?team " for order " ?order " reported!" crlf)
)

(defrule order-recv-ConfirmDelivery
  (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (protobuf-msg (type "llsf_msgs.ConfirmDelivery") (ptr ?p) (rcvd-via STREAM))
  =>
  (if (not (do-for-fact ((?rc referee-confirmation))
            (and (eq ?rc:process-id (pb-field-value ?p "delivery_id"))
                 (eq ?rc:state REQUIRED))
            (if (eq (pb-field-value ?p "correct") TRUE) then
              (modify ?rc (state CONFIRMED))
            else
              (modify ?rc (state DENIED)))
            ; make sure do-for-fact evaluates to TRUE
            TRUE))
   then
    (printout error "Received invalid SetOrderDelivered"
                    " (order " (pb-field-value ?p "order_id")
                    ", team " (pb-field-value ?p "team_color") ")" crlf)
  )
  (retract ?pf)
)

(defrule order-delivery-confirmation-no-operation
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state ~REQUIRED))
  (not (product-processed (id ?id)))
  =>
  (retract ?rf)
  (printout error "Confirmation could not be linked to an operation" crlf)
)

(defrule order-delivery-confirmation-invalid-order
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order))
  (not (order (id ?order) (active TRUE)))
  =>
  (retract ?pf)
  (retract ?rf)
  (assert (attention-message (team ?team)
                             (text (str-cat "Invalid order delivered by " ?team ": "
                             "no active order with ID " ?order crlf))))
)

(defrule order-delivery-confirmation-operation-denied
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state DENIED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE))
   =>
  (retract ?pf)
  (retract ?rf)
  (printout t "Denied delivery for order " ?order  " by team " ?team crlf)
)

(defrule order-delivery-confirmation-operation-confirmed-print
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE))
  (order (id ?order))
   =>
  (printout t "Confirmed delivery for order " ?order  " by team " ?team crlf)
)

(defrule order-delivery-confirmation-operation-confirmed-simulate-tracking
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece 0) (game-time ?delivery-time))
  ?of <- (order (id ?order) (active TRUE))
  (or (workpiece-tracking (enabled FALSE))
      (workpiece-tracking (fail-safe FALSE)))
  =>
  (bind ?wp-id (workpiece-simulate-tracking ?order ?team ?delivery-time))
  (modify ?pf (workpiece ?wp-id) (confirmed TRUE))
  (retract ?rf)
)

(defrule order-delivery-confirmation-operation-confirmed-DS-read-fail-recovery
 "Recover from reading failure at DS, iff there is a unique, fitting, caped unconfirmed, workpiece"
  (declare (salience ?*PRIORITY_HIGH*))
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece 0) (game-time ?delivery-time))
  ?of <- (order (id ?order)
                (active TRUE)
                (base-color ?base-color)
                (ring-colors $?ring-colors))
  (workpiece (id ?wp-id)
             (team ?team)
             (base-color ?base-color)
             (ring-colors $?ring-colors))
  (product-processed (workpiece ?wp-id) (mtype CS) (confirmed FALSE))
  (workpiece-tracking (enabled TRUE))
  (not (and
      (workpiece (team ?team)
                  (base-color ?base-color)
                  (ring-colors $?ring-colors)
                  (id ?wpp-id&:(neq ?wpp-id ?wp-id)))
      (product-processed (workpiece ?wpp-id) (mtype CS) (confirmed FALSE)))
  )
  =>
  (printout t "Delivery for order " ?order  " by team " ?team " not linked to a workpiece" crlf)
  (printout t "Linking to a fitting unique workpiece " ?wp-id   " ready for delivery " crlf)
  (modify ?pf (workpiece ?wp-id))
)

(defrule order-delivery-confirmation-operation-confirmed-DS-read-fail-safe
 "Disable tracking if fai-safe is on and couldn't recover"
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece 0) (game-time ?delivery-time))
  ?of <- (order (id ?order) (active TRUE))
  ?wf <- (workpiece-tracking (enabled TRUE) (fail-safe TRUE))
  =>
  (printout t "Could not find a unique workpiece fitting the delivery " crlf)
  (modify ?wf (enabled FALSE) (reason (str-cat "Fail-safe reader at delivery" )))
)


(defrule order-delivery-confirmation-operation-confirmed-workpiece-rectify
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece ?workpiece-id) (game-time ?delivery-time))
  ?wf <- (workpiece (id ?workpiece-id)
                    (order ?workpiece-order)
                    (cap-color ?workpiece-cap)
                    (base-color ?workpiece-base)
                    (ring-colors $?workpiece-rings))
  ?of <- (order (id ?order)
                (active TRUE)
                (cap-color ?order-cap)
	           (base-color ?order-base)
                (ring-colors $?order-rings))

  (test (or (neq ?workpiece-order ?order)
            (neq ?workpiece-cap ?order-cap)
            (neq ?workpiece-base $?order-base)
            (neq $?workpiece-rings $?order-rings)))
  =>
  (printout t "Verifying operations performed on workpiece " ?workpiece-id  crlf)
  (if (neq ?workpiece-base ?order-base) then
    (if (not (do-for-fact ((?pd product-processed))
                          (and (eq ?pd:mtype BS)
                               (eq ?pd:workpiece ?workpiece-id)
                               (eq ?pd:base-color ?workpiece-base))
                          (modify ?pd (base-color ?order-base) (confirmed TRUE))
                          TRUE))
      then
      (assert (product-processed (mtype BS)
                                 (team ?team)
                                 (order ?order)
                                 (confirmed TRUE)
                                 (workpiece ?workpiece-id)
                                 (game-time ?delivery-time)
                                 (base-color ?order-base)))
    )
    (printout t "Rectifying workpiece " ?workpiece-id ": operation at BS [" ?workpiece-base
                 "->" ?order-base "]"  crlf)
  )
  (if (neq ?workpiece-cap ?order-cap) then
    (if (not (do-for-fact ((?pd product-processed))
                          (and
                               (eq ?pd:mtype CS)
                               (eq ?pd:workpiece ?workpiece-id)
                               (eq ?pd:cap-color ?workpiece-cap))
                          (modify ?pd (cap-color ?order-cap) (confirmed TRUE) (scored FALSE))
                          TRUE))
      then
      (assert (product-processed (mtype CS)
                                 (team ?team)
                                 (scored FALSE)
                                 (confirmed TRUE)
                                 (workpiece ?workpiece-id)
                                 (game-time ?delivery-time)
                                 (cap-color ?order-cap)))
    )
    (printout t "Rectifying workpiece " ?workpiece-id ": operation at CS [" ?workpiece-cap
                 "->" ?order-cap "]"  crlf)
  )
  (if (neq ?order-rings ?workpiece-rings) then
    (progn$ (?order-ring ?order-rings)
       (bind ?workpiece-ring (nth$ ?order-ring-index ?workpiece-rings))
       (if (neq ?workpiece-ring ?order-ring) then
         (if (not (do-for-fact ((?pd product-processed))
                          (and (eq ?pd:mtype RS)
                               (eq ?pd:workpiece ?workpiece-id)
                               (eq ?pd:ring-color ?workpiece-ring))
                          (modify ?pd (ring-color ?order-ring) (confirmed TRUE))
                          TRUE))
            then
            (assert (product-processed (mtype RS)
                                       (team ?team)
                                       (scored FALSE)
                                       (confirmed TRUE)
                                       (workpiece ?workpiece-id)
                                       (game-time ?delivery-time)
                                       (ring-color ?order-ring)))
         )
         (printout t "Rectifying workpiece " ?workpiece-id ": operation at RS [" ?workpiece-ring
                     "->" ?order-ring "]"  crlf)
       )
    )
    ;Retrigger all rings score calculation if a single one is worng
    (do-for-all-facts ((?pd product-processed)) (and (eq ?pd:workpiece ?workpiece-id)
                                                     (eq ?pd:scored TRUE)
                                                     (eq ?pd:mtype RS))
                    (modify ?pd (scored FALSE)))
  )
  (if (neq ?workpiece-order ?order) then
     (printout t "Rectifying workpiece " ?workpiece-id ": order ID corrected [" ?workpiece-order
                 "->" ?order "]"  crlf)
  )
  (do-for-fact ((?pd product-processed)) (and (eq ?pd:workpiece ?workpiece-id)
                                              (eq ?pd:scored TRUE)
                                              (eq ?pd:mtype DS)
                                              (neq ?pd:id ?id))
     (printout t "Rectifying workpiece " ?workpiece-id ": removing old delivery operation for order " ?pd:order  crlf)
     (retract ?pd)
  )
  (modify ?wf (order ?order) (base-color ?order-base) (cap-color ?order-cap) (ring-colors ?order-rings))
)

(defrule order-delivery-confirmation-operation-confirmed-workpiece-verified
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece ?wp-id) (game-time ?delivery-time))
  ?wf <- (workpiece (id ?wp-id)
                    (order ?order)
                    (cap-color ?cap-color)
                    (base-color ?base-color)
                    (ring-colors $?ring-colors))
  ?of <- (order (id ?order)
                (active TRUE)
                (cap-color ?cap-color)
	           (base-color ?base-color)
                (ring-colors $?ring-colors))
   =>
  (printout t "Workpiece " ?wp-id  " verified for order " ?order crlf)
  (modify ?pf (workpiece ?wp-id) (confirmed TRUE))
  (retract ?rf)
)

(defrule order-delivered-correct
	?gf <- (gamestate (phase PRODUCTION|POST_GAME))
	?pf <- (product-processed (game-time ?delivery-time) (team ?team)
	                          (order ?id) (delivery-gate ?gate)
	                          (workpiece ?wp-id) (id ?p-id)
	                          (scored FALSE) (mtype DS)
	                          (confirmed TRUE))
	(not (product-processed (game-time ?other-delivery&:(< ?other-delivery ?delivery-time))
	                        (scored FALSE) (mtype DS)))
  (workpiece (id ?wp-id) (order ?id))
  ; the actual order we are delivering
  ?of <- (order (id ?id) (active TRUE) (complexity ?complexity) (competitive ?competitive)
	        (delivery-gate ?dgate&:(or (eq ?gate 0) (eq ?gate ?dgate)))
	        (base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)
	        (quantity-requested ?q-req) (quantity-delivered $?q-del)
	        (delivery-period $?dp &:(>= ?delivery-time (nth$ 1 ?dp))))
	=>
  (modify ?pf (scored TRUE))
	(bind ?q-del-idx (order-q-del-index ?team))
  (bind ?q-del-new (replace$ ?q-del ?q-del-idx ?q-del-idx (+ (nth$ ?q-del-idx ?q-del) 1)))

  (modify ?of (quantity-delivered ?q-del-new))
  (if (< (nth$ ?q-del-idx ?q-del) ?q-req) then

		; Delivery points
		(bind ?points 0)
		(bind ?reason "")
		(if (<= ?delivery-time (nth$ 2 ?dp)) then
			(bind ?points ?*PRODUCTION-POINTS-DELIVERY*)
			(bind ?reason (str-cat "Delivered item for order " ?id))
		else
			(if (< (- ?delivery-time (nth$ 2 ?dp))
			       ?*PRODUCTION-DELIVER-MAX-LATENESS-TIME*)
			 then
				; 15 - floor(T_d - T_e) * 1.5 + 5
				(bind ?points (integer (+ (- 15 (* (floor (- ?delivery-time (nth$ 2 ?dp))) 1.5)) 5)))
				(bind ?reason (str-cat "Delivered item for order " ?id " (late delivery grace time)"))
			else
				(bind ?points ?*PRODUCTION-POINTS-DELIVERY-TOO-LATE*)
				(bind ?reason (str-cat "Delivered item for order " ?id " (too late delivery)"))
			)
		)
		(assert (points (game-time ?delivery-time) (team ?team) (phase PRODUCTION)
		                (points ?points) (product-step ?p-id) (reason ?reason)))
		(if ?competitive
		 then
			(if (> (nth$ (order-q-del-other-index ?team) ?q-del) (nth$ ?q-del-idx ?q-del))
			 then
				; the other team delivered first
				(bind ?deduction (min ?points ?*PRODUCTION-POINTS-COMPETITIVE-SECOND-DEDUCTION*))
				(assert (points (game-time ?delivery-time) (team ?team) (phase PRODUCTION)
				                (points (* -1 ?deduction)) (product-step ?p-id)
				                (reason (str-cat "Second delivery for competitive order " ?id))))
			 else
				; this team delivered first
				(assert (points (game-time ?delivery-time) (team ?team) (phase PRODUCTION)
				                (points ?*PRODUCTION-POINTS-COMPETITIVE-FIRST-BONUS*)
				                (product-step ?p-id)
				                (reason (str-cat "First delivery for competitive order " ?id))))
			)
		)

   else
    (assert (points (game-time ?delivery-time) (team ?team) (phase PRODUCTION)
		    (points ?*PRODUCTION-POINTS-DELIVERY-WRONG*)
		    (product-step ?p-id)
		    (reason (str-cat "Delivered item for order " ?id))))
  )
)

(defrule order-delivered-wrong-delivgate
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-processed (game-time ?game-time) (team ?team) (mtype DS)
                            (id ?p-id)  (workpiece ?wp-id) (order ?o-id)
                            (scored FALSE) (confirmed TRUE)
                            (delivery-gate ?gate))
  (workpiece (id ?wp-id) (order ?o-id))
  ; the actual order we are delivering
  (order (id ?o-id) (active TRUE) (delivery-gate ?dgate&~?gate&:(neq ?gate 0)))
	=>
  (modify ?pf (scored TRUE))
	(printout warn "Delivered item for order " ?o-id " (wrong delivery gate, got " ?gate ", expected " ?dgate ")" crlf)

	(assert (points (game-time ?game-time) (points ?*PRODUCTION-POINTS-DELIVERY-WRONG*)
									(team ?team) (phase PRODUCTION) (product-step ?p-id)
									(reason (str-cat "Delivered item for order " ?o-id
																	 " (wrong delivery gate)"))))
)

(defrule order-delivered-wrong-too-soon
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-processed (game-time ?game-time) (team ?team) (mtype DS)
                            (id ?p-id) (order ?o-id) (workpiece ?wp-id)
                            (scored FALSE) (confirmed TRUE))
  (workpiece (id ?wp-id) (order ?o-id))
  ; the actual order we are delivering
  (order (id ?o-id) (active TRUE) (delivery-period $?dp&:(< ?game-time (nth$ 1 ?dp))))
	=>
  (modify ?pf (scored TRUE))
	(printout warn "Delivered item for order " ?o-id " (too soon, before time window)" crlf)

	(assert (points (game-time ?game-time) (points 0)
									(team ?team) (phase PRODUCTION) (product-step ?p-id)
									(reason (str-cat "Delivered item for order " ?o-id
																	 " (too soon, before time window)"))))
)

(defrule order-delivered-wrong-too-many
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-processed (game-time ?game-time) (team ?team) (mtype DS)
                            (id ?p-id) (order ?o-id) (workpiece ?wp-id)
                            (scored FALSE) (confirmed TRUE))
  (workpiece (id ?wp-id) (order ?o-id))
  ; the actual order we are delivering
  ?of <- (order (id ?o-id) (active TRUE) (quantity-requested ?q-req)
								(quantity-delivered $?q-del&:(>= (order-q-del-team ?q-del ?team) ?q-req)))
  =>
  (modify ?pf (scored TRUE))
	(printout warn "Delivered item for order " ?o-id " (too many)" crlf)

	(bind ?q-del-idx (order-q-del-index ?team))
  (bind ?q-del-new (replace$ ?q-del ?q-del-idx ?q-del-idx (+ (nth$ ?q-del-idx ?q-del) 1)))
  (modify ?of (quantity-delivered ?q-del-new))

	(assert (points (game-time ?game-time) (points ?*PRODUCTION-POINTS-DELIVERY-WRONG*)
									(team ?team) (phase PRODUCTION) (product-step ?p-id)
									(reason (str-cat "Delivered item for order " ?o-id
																	 " (too many)"))))
)

(defrule order-print-points
  (points (game-time ?gt) (points ?points) (team ?team) (phase ?phase) (reason ?reason))
  =>
  (printout t "Giving " ?points " points to team " ?team ": " ?reason
	    " (" ?phase " @ " ?gt ")" crlf)
)

;-------------------------------------Score for Intermediate
(deffunction order-step-scoring-allowed (?order-id ?team ?q-req ?mtype ?base ?ring ?cap)
   (bind ?scored-for-step
           (find-all-facts ((?p product-processed)) (and (eq ?p:scored TRUE)
                                                       (eq ?p:order ?order-id)
                                                       (eq ?p:team ?team)
                                                       (eq ?p:mtype ?mtype)
                                                       (eq ?p:base-color ?base)
                                                       (eq ?p:ring-color ?ring)
                                                       (eq ?p:cap-color ?cap)
                                                       (eq ?p:confirmed TRUE))))
  (return (> ?q-req (length$ ?scored-for-step)))
)

(defrule order-step-mount-ring
 "Production points for mounting a ring on an intermediate product "
  ?pf <- (product-processed (id ?p-id)
                            (mtype RS)
                            (team ?team)
                            (scored FALSE)
                            (confirmed TRUE)
                            (workpiece ?w-id)
                            (game-time ?g-time)
                            (at-machine ?m-name)
                            (base-color ?step-b-color)
                            (cap-color ?step-c-color)
                            (ring-color ?step-r-color&~nil))
  (workpiece (id ?w-id)
             (team ?team)
             (order ?o-id)
             (base-color ?base-color)
             (ring-colors $?wp-r-colors&:(member$ ?step-r-color
                                                  ?wp-r-colors)))
  (order (id ?o-id)
         (complexity ?complexity)
         (quantity-requested ?q-req)
         (delivery-period $?dp &:(<= ?g-time (nth$ 2 ?dp)))
         (base-color ?base-color)
         (ring-colors $?r-colors&:(eq ?wp-r-colors
                                      (subseq$ ?r-colors 1 (length$ ?wp-r-colors)))))
  (ring-spec (color ?step-r-color) (req-bases ?cc))
  (not (points (product-step ?p-id)))
  (test (order-step-scoring-allowed ?o-id ?team ?q-req RS ?step-b-color ?step-r-color ?step-c-color))
   =>
  ; Production points for ring color complexities
  (bind ?points 0)
    (switch ?cc
        (case 0 then (bind ?points ?*PRODUCTION-POINTS-FINISH-CC0-STEP*))
        (case 1 then (bind ?points ?*PRODUCTION-POINTS-FINISH-CC1-STEP*))
        (case 2 then (bind ?points ?*PRODUCTION-POINTS-FINISH-CC2-STEP*)))
    (assert (points (phase PRODUCTION) (game-time ?g-time) (team ?team)
                    (points ?points) (product-step ?p-id)
                    (reason (str-cat "Mounted CC" ?cc " ring of CC" ?cc
                                       " for order " ?o-id))))
    ; Production points for mounting the last ring (pre-cap points)
    (bind ?complexity-num (length$ ?r-colors))
    (if (eq (nth$ ?complexity-num ?r-colors) ?step-r-color)
    then
    (bind ?pre-cap-points 0)
    (switch ?complexity
      (case C1 then (bind ?pre-cap-points ?*PRODUCTION-POINTS-FINISH-C1-PRECAP*))
      (case C2 then (bind ?pre-cap-points ?*PRODUCTION-POINTS-FINISH-C2-PRECAP*))
      (case C3 then (bind ?pre-cap-points ?*PRODUCTION-POINTS-FINISH-C3-PRECAP*))
    )
        (assert (points (game-time ?g-time) (points ?pre-cap-points)
                        (team ?team) (phase PRODUCTION) (product-step ?p-id)
                        (reason (str-cat "Mounted last ring for complexity "
                                          ?complexity " order " ?o-id))))
  )
  (modify ?pf (scored TRUE) (order ?o-id))
)

(defrule order-step-mount-cap
    "Production points for mounting a cap on an intermediate product "
    ?pf <- (product-processed (id ?p-id)
                              (mtype CS)
                              (team ?team)
                              (scored FALSE)
                              (confirmed TRUE)
                              (workpiece ?w-id)
                              (game-time ?g-time)
                              (at-machine ?m-name)
                              (base-color ?step-b-color)
                              (ring-color ?step-r-color)
                              (cap-color ?step-c-color&~nil))
    (workpiece (id ?w-id)
               (team ?team)
               (order ?o-id)
               (base-color ?base-color)
               (ring-colors $?ring-colors)
               (cap-color ?cap-color&:(eq ?cap-color ?step-c-color)))
    (order (id ?o-id)
           (complexity ?complexity)
           (quantity-requested ?q-req)
           (delivery-period $?dp)
           (base-color ?base-color)
           (ring-colors $?ring-colors)
           (cap-color ?cap-color))
    (not (points (product-step ?p-id)))
    (test (order-step-scoring-allowed ?o-id ?team ?q-req CS ?step-b-color ?step-r-color ?step-c-color))
     =>
     ; Production points for mounting the cap
     (bind ?reason "")
     (bind ?points 0)
     (if (<= ?g-time (nth$ 2 ?dp)) then
       (bind ?reason (str-cat "Mounted cap for order " ?o-id))
       (bind ?points ?*PRODUCTION-POINTS-MOUNT-CAP*)
      else
       (bind ?reason (str-cat "Late cap mount for order " ?o-id " (deadline: " (nth$ 2 ?dp) ")"))
       (bind ?points 0)
     )
     (assert (points (game-time ?g-time) (team ?team)
                     (points ?points)
                     (phase PRODUCTION) (product-step ?p-id)
                     (reason ?reason)))
     (modify ?pf (scored TRUE) (order ?o-id))
)


;-------------------------------fault handling
(defrule order-remove-operations-scored-with-no-workpiece
    "When a workpiece is removed, remove related operations"
    ?pf <- (product-processed (id ?id) (workpiece ?w-id&~0)
                              (confirmed TRUE) (scored TRUE)
                              (mtype ~DS) (team ?team))
    (not (workpiece (id ?w-id)))
    =>
    (retract ?pf)
)

(defrule order-remove-points-of-invalid-operation
   "When operations are removed or scored reset, remove points given for that operation"
   ?pf <- (points (product-step ?id&~0) (points ?points) (reason ?reason))
   (not (product-processed (id ?id) (scored TRUE)))
   =>
   (printout t " Removing " ?points  " points of: " ?reason crlf)
   (retract ?pf)
)

(defrule order-inconsistent-operation-with-workpiece
"Intermediate scored operation became inconsistent its workpiece. This should
 never happen. Operations should always be inline with their workpiece."
    ?pf <- (product-processed (id ?p-id)
                              (team ?team)
                              (mtype ?mtype)
                              (scored TRUE)
                              (confirmed TRUE)
                              (workpiece ?w-id)
                              (game-time ?operation-time)
                              (at-machine ?operation-machine)
                              (base-color ?operation-base)
                              (ring-color ?operation-ring)
                              (cap-color ?operation-cap))
    (workpiece (id ?w-id)
               (team ?team)
               (order ?order)
               (base-color ?base-color)
               (ring-colors $?ring-colors)
               (cap-color ?cap-color))
    (or (and (eq ?mtype BS) (neq ?operation-base ?base-color))
        (and (eq ?mtype CS) (neq ?operation-cap ?cap-color))
        (and (eq ?mtype RS) (not (member$ ?operation-ring $?ring-colors)))
    )
    =>
    (printout warn  "Inconsistent operation at " ?operation-machine
                    " scored for workpiece " ?w-id " " crlf)
)

(defrule order-inconsistent-operation-order
"Retrigger score calculation for operations scored for a different order than
 the one the workpiece is currently tracking."
    ?pf <- (product-processed (id ?p-id)
                              (team ?team)
                              (mtype ?mtype)
                              (scored TRUE)
                              (confirmed TRUE)
                              (workpiece ?w-id)
                              (at-machine ?operation-machine)
                              (base-color ?operation-base)
                              (ring-color ?operation-ring)
                              (cap-color ?operation-cap)
						(order ?operation-order))
    (workpiece (id ?w-id)
               (team ?team)
               (order ?order)
               (cap-color ?cap-color)
               (base-color ?base-color)
               (ring-colors $?ring-colors))
    (order (id ?order) (active TRUE))
    (test (or (and (eq ?mtype CS) (eq ?operation-cap ?cap-color))
              (and (eq ?mtype RS) (member$ ?operation-ring $?ring-colors))))
    (test (neq ?operation-order ?order))
    =>
    (modify ?pf (order ?order) (scored FALSE))
    (printout warn "Workpiece " ?w-id  " currently tracks order " ?order
                    " instead of " ?operation-order
                     ", Canceling operation at " ?operation-machine  crlf)
)

