
;---------------------------------------------------------------------------
;  setup.clp - LLSF RefBox CLIPS setup phase rules
;
;  Created: Thu Jun 13 11:24:21 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule setup-speedup-light
  (gamestate (phase SETUP) (state RUNNING) (game-time ?gt&:(>= ?gt 240)))
  =>
  (bind ?*SETUP-LIGHT-PERIOD* 0.5)
)

(defrule setup-speedup-light-more
  (gamestate (phase SETUP) (state RUNNING) (game-time ?gt&:(>= ?gt 270)))
  =>
  (bind ?*SETUP-LIGHT-PERIOD* 0.25)
)

(defrule setup-toggle-light
  (time $?now)
  ?f <- (signal (type setup-light-toggle)
		(time $?t&:(timeout ?now ?t ?*SETUP-LIGHT-PERIOD*)))
  (gamestate (phase SETUP) (state RUNNING))
  ?sf <- (setup-light-toggle ?m)
  =>
  (modify ?f (time ?now))
  (retract ?sf)
  (bind ?next-m (+ (mod ?m 10) 1))
  (assert(setup-light-toggle ?next-m))

  (do-for-fact ((?machine machine)) (eq ?machine:name (sym-cat M ?m))
    (modify ?machine (desired-lights RED-ON YELLOW-ON GREEN-ON))
  )
  (do-for-fact ((?machine machine)) (eq ?machine:name (sym-cat M ?next-m))
    (modify ?machine (desired-lights))
  )
)
