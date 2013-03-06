
;---------------------------------------------------------------------------
;  orders.clp - LLSF RefBox CLIPS order processing
;
;  Created: Sun Feb 24 19:40:32 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule activate-order
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?of <- (order (active FALSE) (activate-at ?at&:(>= ?gt ?at))
		(product ?p) (quantity-requested ?q) (delivery-period $?period))
  ?sf <- (signal (type order-info))
  =>
  (modify ?of (active TRUE))
  (modify ?sf (count 1) (time 0 0))
  (assert (attention-message (str-cat "Late order: " ?q " x " ?p " from "
				      (nth$ 1 ?period) " to " (nth$ 2 ?period)) 15))
)

; Sort orders by ID, such that do-for-all-facts on the orders deftemplate
; iterates in a nice order, e.g. for net-send-OrderIntstruction
(defrule sort-orders
  (declare (salience ?*PRIORITY_HIGH*))
  ?oa <- (order (id ?id-a))
  ?ob <- (order (id ?id-b&:(> ?id-a ?id-b)&:(< (fact-index ?oa) (fact-index ?ob))))
  =>
  (modify ?ob)
  (modify ?oa)
)

(defrule order-delivered-in-time
  ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt) (points ?points))
  ?pf <- (product-delivered (time $?time) (product ?p) (delivery-gate ?dg))
  ; the actual order we are delivering
  ?of <- (order (id ?oid) (active TRUE) (product ?p) (quantity-requested ?q-req)
		(points ?order-points) (points-supernumerous ?order-points-supernumerous)
		(quantity-delivered ?q-del) (delivery-gate ?odg&?dg|ANY)
		(delivery-period $?dp&:(>= ?gt (nth$ 1 ?dp))&:(<= ?gt (nth$ 2 ?dp))))

    ; There is no other order, which would
  (not (or
    ; give more points, is active, is in-time, and still goods requested
    (order (id ?oid2&:(<> ?oid ?oid2)) (active TRUE) (product ?p)
	   (delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2)))
	   (delivery-gate ?odg2&?dg|ANY) (quantity-delivered ?q-del-2)
	   (points ?op2&:(> ?op2 ?order-points))
	   (quantity-requested ?q-req-2&:(< ?q-del-2 ?q-req-2)))
    ; give more points than supernumerous points, is active, is in-time, and that has
    ; goods requested while the chosen order has not
    (order (id ?oid2&:(<> ?oid ?oid2)) (active TRUE) (product ?p)
	   (delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2)))
	   (delivery-gate ?odg2&?dg|ANY) (quantity-delivered ?q-del-2)
	   (points-supernumerous ?op2&:(> ?order-points ?op2))
	   (quantity-requested ?q-req-2&:(< ?q-del-2 ?q-req-2 )&:(>= ?q-del ?q-req)))
    ; give more supernumerous points, is active, is in-time, and that has
    ; all products delivered while the chosen order has as well
    (order (id ?oid2&:(<> ?oid ?oid2)) (active TRUE) (product ?p)
	   (delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2)))
	   (delivery-gate ?odg2&?dg|ANY) (quantity-delivered ?q-del-2)
	   (points-supernumerous ?op2&:(> ?op2 ?order-points-supernumerous))
	   (quantity-requested ?q-req-2&:(>= ?q-del-2 ?q-req-2 )&:(>= ?q-del ?q-req)))
  ))
  =>
  (retract ?pf)
  (modify ?of (quantity-delivered (+ ?q-del 1)))
  (bind ?addp (if (< ?q-del ?q-req) then ?order-points else ?order-points-supernumerous))
  (modify ?gf (points (+ ?points ?addp)))
  (printout t "Product " ?p " delivered at " ?dg ". Awarding " ?addp " points" crlf)
)
