
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

;; net.clp related functions and rules for websocket communciation

; OrderInfo

(defrule ws-proc-OrderInfo
     ?msg <- (ws-send-OrderInfo)
     =>
     (retract ?msg)
     (ws-create-OrderInfo)
)

; WorkpieceInfo

(defrule ws-proc-send-WorkpieceInfo 
  ?msg <- (ws-send-WorkpieceInfo)
  =>
  (retract ?msg)
  (ws-create-WorkpieceInfo)
)

; RingInfo

(defrule ws-proc-send-RingInfo
  ?msg <-(ws-send-RingInfo)
  =>
  (retract ?msg)
  (ws-create-RingInfo)
)

; MachineInfo

(defrule ws-proc-send-MachineInfo
  ?msg <- (ws-send-MachineInfo)
  =>
  (retract ?msg)
  (ws-create-MachineInfo)
)

; RobotInfo

(defrule ws-proc-send-RobotInfo
  ?msg <- (ws-send-RobotInfo)
  =>
  (retract ?msg)
  (ws-create-RobotInfo)
)

; GameState

(defrule ws-proc-send-GameState
  ;?msg <- (ws-send-GameState)
  ?gs <- (gamestate (game-time ?gt))
  =>
  ;(retract ?msg)
  (ws-create-GameState)  
)