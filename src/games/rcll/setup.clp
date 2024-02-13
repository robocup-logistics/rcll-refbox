
;---------------------------------------------------------------------------
;  setup.clp - LLSF RefBox CLIPS setup phase rules
;
;  Created: Thu Jun 13 11:24:21 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule setup-speedup-light
  (gamestate (phase SETUP) (state RUNNING))
  (time-info (game-time ?gt&:(>= ?gt ?*SETUP-LIGHT-SPEEDUP-TIME-1*)))
  =>
  (bind ?*SETUP-LIGHT-PERIOD* ?*SETUP-LIGHT-PERIOD-1*)
)

(defrule setup-speedup-light-more
  (gamestate (phase SETUP) (state RUNNING))
  (time-info (game-time ?gt&:(>= ?gt ?*SETUP-LIGHT-SPEEDUP-TIME-2*)))
  =>
  (bind ?*SETUP-LIGHT-PERIOD* ?*SETUP-LIGHT-PERIOD-2*)
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
  (bind ?n (+ (mod (member$ ?m ?*SETUP-LIGHT-MACHINES*) (length$ ?*SETUP-LIGHT-MACHINES*)) 1))
  (bind ?next-m (nth$ ?n ?*SETUP-LIGHT-MACHINES*))
  (assert (setup-light-toggle ?next-m))

  ; Turn on previous machines again
  (delayed-do-for-all-facts ((?ml machine-lights))
    (or (eq ?ml:name (sym-cat C- ?m)) (eq ?ml:name (sym-cat M- ?m)))
    (modify ?ml (desired-lights RED-ON YELLOW-ON GREEN-ON))
  )

  ; Turn off current machines
  (delayed-do-for-all-facts ((?ml machine-lights))
    (or (eq ?ml:name (sym-cat C- ?next-m))
	(eq ?ml:name (sym-cat M- ?next-m)))

    (modify ?ml (desired-lights))
  )
)
