
;---------------------------------------------------------------------------
;  websocket.clp - LLSF RefBox CLIPS fact monitoring for websocket clients
;
;  Created: Mon May 11 15:19:25 2020
;  Copyright  2020       Daniel Swoboda
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule ws-monitor-gamephase                                                     
    ?gs <- (gamestate (phase ?phase) (state ?state) (game-time ?gt) (teams ?team_cyan ?team_magenta) (prev-phase ?prevphase) (points ?points-cyan ?points-magenta))             
    =>         
    ; whenever the gamestate changes, call signal function to transmit the changes to the connected clients                                                  
    (ws-push-gamestate (str-cat ?gt) (str-cat ?state) (str-cat ?phase) (str-cat ?prevphase) (str-cat ?team_cyan) (str-cat ?team_magenta) )                     
)

(defrule ws-monitor-gamepoints
    ?gs <- (gamestate (points ?points-cyan ?points-magenta))
    =>
    (ws-push-gamepoints (str-cat ?points-cyan) (str-cat ?points-magenta))
)
