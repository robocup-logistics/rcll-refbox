
;---------------------------------------------------------------------------
;  websocket.clp - LLSF RefBox CLIPS fact monitoring for websocket clients
;
;  Created: Mon May 11 15:19:25 2020
;  Copyright  2020       Daniel Swoboda
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

; forward attention messages to the websocket backend

(defrule ws-send-attention-message
     ?msg <- (ws-attention-message ?text ?team ?time-to-show)
     =>
     (retract ?msg)
     (ws-send-attention-message (str-cat ?text) (str-cat ?team) (str-cat ?time-to-show))
)

; handle machine reset by team 

(defrule ws-reset-machine-by-team
  ?msg <- (ws-reset-machine-message ?machine ?team_color)
  =>
  (retract ?msg)
  (printout t "Received reset for " ?machine crlf)
  (do-for-fact ((?m machine)) (and (eq ?m:name ?machine) (eq ?m:team ?team_color))
    (modify ?m (state BROKEN)
      (broken-reason (str-cat "Machine " ?machine " resetted by the team " ?team_color)))
  )
)

;; message generation based on fact changes

; send udpate of gamestate whenever the gamestate fact changes

(defrule ws-update-gamestate
  ?sf <- (gamestate)
  =>
  (ws-create-GameState)  
)

; send update of an order, whenever the order fact changes

(defrule ws-update-order
  ?sf <- (order (id ?id))
  =>
  (ws-create-OrderInfo ?id)
)

; send update of an order, whenever the unconfirmed delivery information changes

(defrule ws-update-unconfirmed-delivery
  ?sf <- (product-processed (order ?id))
  =>
  (ws-create-OrderInfo ?id)
)

; send an update when the fact ws-update-order-cmd is asserted by an external rule or function

(defrule ws-update-order-external
  ?cmd <- (ws-update-order-cmd ?id)
  =>
  (retract ?cmd)
  (ws-create-OrderInfo ?id)
)


; send update of a robot, whenever the robot fact changes

(defrule ws-update-robot
  ?sf <- (robot (number ?number) (name ?name))
  =>
  (ws-create-RobotInfo ?number ?name)
)

; send update of a workpiece, whenever the workpiece fact changes

(defrule ws-update-workpiece
  ?sf <- (workpiece (id ?id))
  =>
  (ws-create-WorkpieceInfo ?id)
)

; send update of a machine, whenever the machine fact changes

(defrule ws-update-machine
  ?sf <- (machine (name ?name))
  =>
  (ws-create-MachineInfo (str-cat ?name))
)

; send update of points, whenever a points fact changes

(defrule ws-update-points
  ?sf <- (points)
  =>
  (ws-create-Points)
)

; send update of ring-spec, whenever a ring-spec fact changes

(defrule ws-update-ringspec
  ?sf <- (ring-spec)
  =>
  (ws-create-RingInfo)
)