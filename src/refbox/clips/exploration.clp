
;---------------------------------------------------------------------------
;  exploration.clp - LLSF RefBox CLIPS exploration phase rules
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule machine-enable-exploration
  ?gf <- (gamestate (phase EXPLORATION) (prev-phase ~EXPLORATION))
  ?mf <- (machine (mtype ?mtype))
  (machine-spec (mtype ?mtype) (light-code ?lc))
  =>
  (modify ?gf (prev-phase EXPLORATION))
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (bind ?dl (create$))
    (do-for-fact ((?spec machine-spec)) (eq ?machine:mtype ?spec:mtype)
      (bind ?dl ?spec:light-code)
    )
    (modify ?machine (desired-lights ?dl))
  )
)
