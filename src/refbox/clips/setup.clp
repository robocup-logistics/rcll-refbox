
;---------------------------------------------------------------------------
;  setup.clp - LLSF RefBox CLIPS setup phase rules
;
;  Created: Thu Jun 13 11:24:21 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule setup-speedup-light
  (gamestate (phase SETUP) (state RUNNING)
	     (game-time ?gt&:(>= ?gt ?*SETUP-LIGHT-SPEEDUP-TIME-1*)))
  =>
  (bind ?*SETUP-LIGHT-PERIOD* ?*SETUP-LIGHT-PERIOD-1*)
)

(defrule setup-speedup-light-more
  (gamestate (phase SETUP) (state RUNNING)
	     (game-time ?gt&:(>= ?gt ?*SETUP-LIGHT-SPEEDUP-TIME-2*)))
  =>
  (bind ?*SETUP-LIGHT-PERIOD* ?*SETUP-LIGHT-PERIOD-2*)
)

(defrule setup-toggle-light
  (time $?now)
  ?f <- (signal (type setup-light-toggle)
		(time $?t&:(timeout ?now ?t ?*SETUP-LIGHT-PERIOD*)))
  (gamestate (phase SETUP) (state RUNNING))
  ?sf <- (setup-light-toggle ?m1 ?m2)
  =>
  (modify ?f (time ?now))
  (retract ?sf)
  (bind ?next-m1 (+ (mod ?m1 12) 1))
  (bind ?next-m2 (+ ?next-m1 12))
  (assert (setup-light-toggle ?next-m1 ?next-m2))

  ; Turn on previous machines again
  (delayed-do-for-all-facts ((?machine machine))
    (or (eq ?machine:name (sym-cat M ?m1)) (eq ?machine:name (sym-cat M ?m2)))

    (modify ?machine (desired-lights RED-ON YELLOW-ON GREEN-ON))
  )

  ; Turn off current machines
  (delayed-do-for-all-facts ((?machine machine))
    (or (eq ?machine:name (sym-cat M ?next-m1))
	(eq ?machine:name (sym-cat M ?next-m2)))

    (modify ?machine (desired-lights))
  )
)
