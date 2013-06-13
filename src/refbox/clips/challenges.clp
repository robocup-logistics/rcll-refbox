
;---------------------------------------------------------------------------
;  challenges.clp - LLSF RefBox CLIPS technical challenges
;
;  Created: Thu Jun 13 13:12:44 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------


; ***** WHACK-A-MOLE challenge *****
(defrule techal-wam-next
  ?gs <- (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING) (prev-state ~RUNNING))
  ?wf <- (whac-a-mole-light ?old-machine)
  =>
  (retract ?wf)
  (modify ?gs (prev-state RUNNING))

  (bind ?old-idx (member$ ?old-machine ?*TECHCHALL-WAM-MACHINES*))
  (bind ?candidates ?*TECHCHALL-WAM-MACHINES*)
  (if ?old-idx then (bind ?candidates (delete$ ?candidates ?old-idx ?old-idx)))

  (bind ?new-idx (random 1 (length$ ?candidates)))
  (bind ?new-machine (nth$ ?new-idx ?candidates))

  (if (neq ?old-machine NONE)
  then
    (printout t "Machine " ?old-machine " reached, next is " ?new-machine crlf)
  else
    (printout t "Starting with machine " ?new-machine crlf)
  )
  (assert (whac-a-mole-light ?new-machine))
)

(defrule techal-wam-deactivate
  (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING))
  (whac-a-mole-light ?m)
  ?mf <- (machine (name ~?m) (state ~DOWN))
  =>
  (modify ?mf (state DOWN) (desired-lights))
)

(defrule techal-wam-activate
  (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING))
  (whac-a-mole-light ?m)
  ?mf <- (machine (name ?m) (state ~IDLE))
  =>
  (modify ?mf (state IDLE) (desired-lights RED-ON YELLOW-ON GREEN-ON))
)

(defrule techal-wam-continue
  ?gs <- (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state PAUSED) (prev-state RUNNING)
		    (game-time ?gtime))
  (whac-a-mole-light ?m)
  =>
  (assert (points (game-time ?gtime) (points 1) (phase WHACK_A_MOLE_CHALLENGE)
		  (reason (str-cat "Machine " ?m " reached (referee)"))))

  (modify ?gs (state RUNNING) (prev-state PAUSED))
)

(defrule techal-wam-reached
  ?gs <- (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING) (game-time ?gtime))
  (whac-a-mole-light ?m)
  ?mf <- (machine (name ?m) (pose $?m-pose))
  (robot (vision-pose $?r-pose&:(in-box ?r-pose ?m-pose ?*TECHCHALL-WAM-BOX-SIZE*)))
  =>
  (assert (points (game-time ?gtime) (points 1) (phase WHACK_A_MOLE_CHALLENGE)
		  (reason (str-cat "Machine " ?m " reached (vision)"))))
  (modify ?gs (state RUNNING) (prev-state PAUSED))
)

(defrule techal-wam-game-over
  ?gs <- (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING)
		    (game-time ?game-time&:(>= ?game-time ?*TECHCHALL-WAM-TIME*)))
  =>
  (modify ?gs (phase POST_GAME) (prev-phase WHACK_A_MOLE_CHALLENGE) (state PAUSED)
	  (end-time (now)))
)


; ***** NAVIGATION challenge *****
(defrule techal-navigation-game-over
  ?gs <- (gamestate (phase NAVIGATION_CHALLENGE) (state RUNNING)
		    (game-time ?game-time&:(>= ?game-time ?*TECHCHALL-NAVIGATION-TIME*)))
  =>
  (modify ?gs (phase POST_GAME) (prev-phase NAVIGATION_CHALLENGE) (state PAUSED) (end-time (now)))
)
