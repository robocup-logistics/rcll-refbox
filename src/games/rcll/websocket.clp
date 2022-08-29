
;---------------------------------------------------------------------------
;  websocket.clp - RCLL RefBox CLIPS fact monitoring for websocket clients
;
;  Created: Mon May 11 15:19:25 2020
;  Copyright  2020       Daniel Swoboda
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule ws-send-attention-message
  "forward attention messages to the websocket backend"
  ?msg <- (ws-attention-message ?text ?team ?time-to-show)
  =>
  (retract ?msg)
  (ws-send-attention-message (str-cat ?text) (str-cat ?team) (str-cat ?time-to-show))
)

(defrule ws-reset-machine-by-team
  "handle machine reset by team"
  ?msg <- (ws-reset-machine-message ?machine ?team_color)
  =>
  (retract ?msg)
  (printout t "Received reset for " ?machine crlf)
  (do-for-fact ((?m machine)) (and (eq ?m:name ?machine) (eq ?m:team ?team_color))
    (modify ?m (state BROKEN)
      (broken-reason (str-cat "Machine " ?machine " resetted by the team " ?team_color)))
  )
)

; message generation based on fact changes

(defrule ws-update-gamestate
  "send udpate of gamestate whenever the gamestate fact changes"
  ?sf <- (gamestate)
  =>
  (ws-create-GameState)
)

(defrule ws-update-order
  "send update of an order, whenever the order fact changes"
  ?sf <- (order (id ?id))
  =>
  (ws-create-OrderInfo ?id)
)

(defrule ws-update-unconfirmed-delivery
  "send update of an order, whenever the unconfirmed delivery information changes"
  ?sf <- (product-processed (order ?id))
  =>
  (ws-create-OrderInfo ?id)
)

(defrule ws-update-order-external
  "send an update when the fact ws-update-order-cmd is asserted by an external rule or function"
  ?cmd <- (ws-update-order-cmd ?id)
  =>
  (retract ?cmd)
  (ws-create-OrderInfo-via-delivery ?id)
)

(defrule ws-update-robot
  "send update of a robot, whenever the robot fact changes"
  ?sf <- (robot (number ?number) (name ?name))
  =>
  (ws-create-RobotInfo ?number ?name)
)

(defrule ws-update-workpiece
  "send update of a workpiece, whenever the workpiece fact changes"
  ?sf <- (workpiece (id ?id) (latest-data TRUE))
  =>
  (ws-create-WorkpieceInfo ?id)
)

(defrule ws-update-machine
  "send update of a machine, whenever the machine fact changes"
  ?sf <- (machine (name ?name))
  =>
  (ws-create-MachineInfo (str-cat ?name))
)

(defrule ws-update-points
  "send update of points, whenever a points fact changes"
  ?sf <- (points)
  =>
  (ws-create-Points)
)

(defrule ws-update-ringspec
  "send update of ring-spec, whenever a ring-spec fact changes"
  ?sf <- (ring-spec)
  =>
  (ws-create-RingInfo)
)
