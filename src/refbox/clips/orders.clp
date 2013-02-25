
;---------------------------------------------------------------------------
;  orders.clp - LLSF RefBox CLIPS order processing
;
;  Created: Sun Feb 24 19:40:32 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule activate-order
  (gamestate (game-time ?gt))
  ?of <- (order (active FALSE) (activate-at ?at&:(>= ?gt ?at))
		(product ?p) (quantity-requested ?q) (delivery-period $?period))
  =>
  (modify ?of (active TRUE))
  (assert (attention-message (str-cat "New order: " ?q " x " ?p " from "
				      (nth$ 1 ?period) " to " (nth$ 2 ?period)) 5))
)
