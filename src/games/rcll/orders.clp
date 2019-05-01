
;---------------------------------------------------------------------------
;  orders.clp - LLSF RefBox CLIPS order processing
;
;  Created: Sun Feb 24 19:40:32 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
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
  (assert (product-delivered (game-time ?gt) (team ?team) (order ?order) (confirmed TRUE)))
  (printout t "Delivery by team " ?team " for order " ?order " reported!" crlf)
)

(defrule order-recv-ConfirmDelivery
  (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (protobuf-msg (type "llsf_msgs.ConfirmDelivery") (ptr ?p) (rcvd-via STREAM))
  =>
  (if (not (do-for-fact
            ((?pd product-delivered))
            (and (eq ?pd:id (pb-field-value ?p "delivery_id"))
                 (eq ?pd:confirmed FALSE))
            (if (eq (pb-field-value ?p "correct") TRUE) then
              (printout t "Correct delivery for order " ?pd:order
                          " by team " ?pd:team crlf)
              (modify ?pd (confirmed TRUE))
            else
              (printout t "Incorrect delivery for order " ?pd:order
                          " by team " ?pd:team crlf)
              (retract ?pd))
            ; make sure do-for-fact evaluates to TRUE
            TRUE))
   then
    (printout error "Received invalid SetOrderDelivered"
                    " (order " (pb-field-value ?p "order_id")
                    ", team " (pb-field-value ?p "team_color") ")" crlf)
  )
  (retract ?pf)
)


(defrule order-delivered-correct
	?gf <- (gamestate (phase PRODUCTION|POST_GAME))
	?pf <- (product-delivered (game-time ?delivery-time) (team ?team)
	                          (order ?id&~0) (delivery-gate ?gate)
	                          (confirmed TRUE))
	(not (product-delivered (game-time ?other-delivery&:(< ?other-delivery ?delivery-time))))
  ; the actual order we are delivering
  ?of <- (order (id ?id) (active TRUE) (complexity ?complexity) (competitive ?competitive)
	        (delivery-gate ?dgate&:(or (eq ?gate 0) (eq ?gate ?dgate)))
	        (base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)
	        (quantity-requested ?q-req) (quantity-delivered $?q-del)
	        (delivery-period $?dp &:(>= ?delivery-time (nth$ 1 ?dp))))
	=>
  (retract ?pf)
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
		                (points ?points) (reason ?reason)))
		(if ?competitive
		 then
			(if (> (nth$ (order-q-del-other-index ?team) ?q-del) (nth$ ?q-del-idx ?q-del))
			 then
				; the other team delivered first
				(bind ?deduction (min ?points ?*PRODUCTION-POINTS-COMPETITIVE-SECOND-DEDUCTION*))
				(assert (points (game-time ?delivery-time) (team ?team) (phase PRODUCTION)
				                (points (* -1 ?deduction))
				                (reason (str-cat "Second delivery for competitive order " ?id))))
			 else
				; this team delivered first
				(assert (points (game-time ?delivery-time) (team ?team) (phase PRODUCTION)
				                (points ?*PRODUCTION-POINTS-COMPETITIVE-FIRST-BONUS*)
				                (reason (str-cat "First delivery for competitive order " ?id))))
			)
		)

		; Production points for mounting the cap
    (assert (points (game-time ?delivery-time) (points ?*PRODUCTION-POINTS-MOUNT-CAP*)
		    (team ?team) (phase PRODUCTION)
		    (reason (str-cat "Mounted cap for order " ?id))))

   else
    (assert (points (game-time ?delivery-time) (team ?team) (phase PRODUCTION)
		    (points ?*PRODUCTION-POINTS-DELIVERY-WRONG*)
		    (reason (str-cat "Delivered item for order " ?id))))
  )
)

(defrule order-delivered-invalid
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-delivered (game-time ?game-time) (team ?team) (order ?order))
	(not (order (id ?order)))
	=>
	(retract ?pf)
	(assert (attention-message (team ?team)
                             (text (str-cat "Invalid order delivered by " ?team ": "
                                            "no active order with ID "
                                            ?order crlf))))
)

(defrule order-delivered-wrong-delivgate
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-delivered (game-time ?game-time) (team ?team)
                            (delivery-gate ?gate) (order ?id) (confirmed TRUE))
  ; the actual order we are delivering
  (order (id ?id) (active TRUE) (delivery-gate ?dgate&~?gate&:(neq ?gate 0)))
	=>
  (retract ?pf)
	(printout warn "Delivered item for order " ?id " (wrong delivery gate, got " ?gate ", expected " ?dgate ")" crlf)

	(assert (points (game-time ?game-time) (points ?*PRODUCTION-POINTS-DELIVERY-WRONG*)
									(team ?team) (phase PRODUCTION)
									(reason (str-cat "Delivered item for order " ?id
																	 " (wrong delivery gate)"))))
)

(defrule order-delivered-wrong-too-soon
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-delivered (game-time ?game-time) (team ?team) (order ?id)
                            (confirmed TRUE))
  ; the actual order we are delivering
  (order (id ?id) (active TRUE) (delivery-period $?dp&:(< ?game-time (nth$ 1 ?dp))))
	=>
  (retract ?pf)
	(printout warn "Delivered item for order " ?id " (too soon, before time window)" crlf)

	(assert (points (game-time ?game-time) (points 0)
									(team ?team) (phase PRODUCTION)
									(reason (str-cat "Delivered item for order " ?id
																	 " (too soon, before time window)"))))
)

(defrule order-delivered-wrong-too-many
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-delivered (game-time ?game-time) (team ?team) (order ?id)
                            (confirmed TRUE))
  ; the actual order we are delivering
  ?of <- (order (id ?id) (active TRUE)
								(quantity-requested ?q-req)
								(quantity-delivered $?q-del&:(>= (order-q-del-team ?q-del ?team) ?q-req)))
  =>
  (retract ?pf)
	(printout warn "Delivered item for order " ?id " (too many)" crlf)

	(bind ?q-del-idx (order-q-del-index ?team))
  (bind ?q-del-new (replace$ ?q-del ?q-del-idx ?q-del-idx (+ (nth$ ?q-del-idx ?q-del) 1)))
  (modify ?of (quantity-delivered ?q-del-new))

	(assert (points (game-time ?game-time) (points ?*PRODUCTION-POINTS-DELIVERY-WRONG*)
									(team ?team) (phase PRODUCTION)
									(reason (str-cat "Delivered item for order " ?id
																	 " (too many)"))))
)

(defrule order-print-points
  (points (game-time ?gt) (points ?points) (team ?team) (phase ?phase) (reason ?reason))
  =>
  (printout t "Giving " ?points " points to team " ?team ": " ?reason
	    " (" ?phase " @ " ?gt ")" crlf)
)

;-------------------------------------Score for Intermediate
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
                            (ring-color ?r-color&~nil))
  (workpiece (id ?w-id)
             (order ?o-id)
             (base-color ?base-color)
             (ring-colors $?wp-r-colors&:(member$ ?r-color
                                                  ?wp-r-colors)))
  (order (id ?o-id)
         (complexity ?complexity)
         (quantity-requested ?q-req)
         (delivery-period $?dp &:(<= ?g-time (nth$ 2 ?dp)))
         (base-color ?base-color)
         (ring-colors $?r-colors&:(eq ?wp-r-colors
                                      (subseq$ ?r-colors 1 (length$ ?wp-r-colors)))))
  (ring-spec (color ?r-color) (req-bases ?cc))
  (not (points (product-step ?p-id)))
   =>
  ; Production points for ring color complexities
  (bind ?points 0)
    (switch ?cc
        (case 0 then (bind ?points ?*PRODUCTION-POINTS-FINISH-CC0-STEP*))
        (case 1 then (bind ?points ?*PRODUCTION-POINTS-FINISH-CC1-STEP*))
        (case 2 then (bind ?points ?*PRODUCTION-POINTS-FINISH-CC2-STEP*))   U)
    (assert (points (phase PRODUCTION) (game-time ?g-time) (team ?team)
                    (points ?points) (product-step ?p-id)
                    (reason (str-cat "Mounted CC" ?cc " ring of CC" ?cc
                                       " for order " ?o-id))))
    ; Production points for mounting the last ring (pre-cap points)
    (bind ?complexity-num (length$ ?r-colors))
    (if (eq (nth$ ?complexity-num ?r-colors) ?r-color)
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
