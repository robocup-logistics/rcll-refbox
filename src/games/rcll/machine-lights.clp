;---------------------------------------------------------------------------
;  machine-lights.clp - Set the MPS light signals according to the state
;
;  Created: Tue 28 May 2019 21:23:16 CEST
;  Copyright  2019  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
;  Licensed under GPLv2+ license, cf. LICENSE file in the doc directory.
;---------------------------------------------------------------------------

; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU Library General Public License for more details.
;
; Read the full text in the LICENSE.GPL file in the doc directory.
;
(defrule machine-report-init-lights
	(declare (salience ?*PRIORITY_HIGH*))
	?gf <- (gamestate (phase ?PHASE) (prev-phase SETUP))
	=>
	; Set prev phase to avoid re-firing, reset game time
	(modify ?gf (prev-phase ?PHASE) (game-time 0.0))

	; Set lights
	(delayed-do-for-all-facts ((?machine machine)) TRUE
		(modify ?machine (desired-lights (create$ YELLOW-ON)))
	)
)

(defrule machine-lights-pause
	(gamestate (phase EXPLORATION|PRODUCTION) (state PAUSED) (prev-state ~PAUSED))
	?m <- (machine (desired-lights ?some-light))
	=>
	(delayed-do-for-all-facts ((?machine machine)) TRUE
		(modify ?machine (desired-lights))
	)
)

(defrule machine-lights-unknown-report
	(gamestate (phase EXPLORATION|PRODUCTION) (state RUNNING))
	?m <- (machine (name ?n) (desired-lights ?dl&:(neq $?dl (create$ YELLOW-ON))))
	(exploration-report (name ?n) (correctly-reported UNKNOWN) (zone NOT-REPORTED))
	=>
	(modify ?m (desired-lights (create$ YELLOW-ON)))
)

(defrule machine-lights-partial-report
	(gamestate (phase EXPLORATION|PRODUCTION) (state RUNNING))
	?m <- (machine (name ?n) (desired-lights ?dl&:(neq $?dl YELLOW-BLINK)))
	(exploration-report (name ?n) (correctly-reported UNKNOWN)
	                    (zone-state CORRECT_REPORT))
	=>
	(printout t (neq ?dl (create$ YELLOW-BLINK)) " and " (neq ?dl YELLOW-BLINK) crlf)
	(modify ?m (desired-lights (create$ YELLOW-BLINK)))
)

(defrule machine-lights-wrong-report-zone
	(gamestate (phase EXPLORATION|PRODUCTION) (state RUNNING))
	?m <- (machine (name ?n) (desired-lights ?dl&:(neq $?dl (create$ RED-ON YELLOW-ON))))
	(exploration-report (name ?n) (zone-state WRONG_REPORT))
	=>
	(modify ?m (desired-lights (create$ RED-ON YELLOW-ON)))
)

(defrule machine-lights-wrong-report-rotation
	(gamestate (phase EXPLORATION|PRODUCTION) (state RUNNING))
	?m <- (machine (name ?n) (desired-lights ?dl&:(neq $?dl (create$ RED-ON YELLOW-BLINK))))
	(exploration-report (name ?n) (rotation-state WRONG_REPORT))
	=>
	(modify ?m (desired-lights (create$ RED-ON YELLOW-BLINK)))
)

(defrule machine-lights-idle
	(gamestate (phase PRODUCTION))
	?m <- (machine (state IDLE) (name ?n) (desired-lights $?dl&:(neq ?dl (create$ GREEN-ON))))
	(exploration-report (name ?n) (correctly-reported TRUE))
	=>
	(modify ?m (desired-lights GREEN-ON))
)

(defrule machine-lights-wait-idle
	(gamestate (phase PRODUCTION))
	?m <- (machine (name ?n) (state WAIT-IDLE) (desired-lights $?dl&:(neq ?dl (create$ YELLOW-BLINK GREEN-BLINK))))
	(exploration-report (name ?n) (correctly-reported TRUE))
	=>
	(modify ?m (desired-lights (create$ YELLOW-BLINK GREEN-BLINK)))
)

(defrule machine-lights-down
	(gamestate (phase PRODUCTION))
	?m <- (machine (name ?n) (state DOWN) (desired-lights $?dl&:(neq ?dl (create$ RED-ON))))
	(exploration-report (name ?n) (correctly-reported TRUE))
	=>
	(modify ?m (desired-lights (create$ RED-ON)))
)

(defrule machine-lights-processing
	(gamestate (phase PRODUCTION))
	?m <- (machine (name ?n) (state PROCESSING) (desired-lights $?dl&:(neq ?dl (create$ YELLOW-ON GREEN-ON))))
	(exploration-report (name ?n) (correctly-reported TRUE))
	=>
	(modify ?m (desired-lights YELLOW-ON GREEN-ON))
)

(defrule machine-lights-prepared
	(gamestate (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (state PREPARED) (desired-lights $?dl&:(neq ?dl (create$ GREEN-BLINK))))
	(exploration-report (name ?n) (correctly-reported TRUE))
	=>
	(modify ?m (desired-lights GREEN-BLINK) (prep-blink-start ?gt))
)

(defrule machine-lights-ready-at-output
	(gamestate (phase PRODUCTION))
	?m <- (machine (name ?n) (state READY-AT-OUTPUT) (desired-lights $?dl&:(neq ?dl (create$ YELLOW-ON GREEN-BLINK))))
	(exploration-report (name ?n) (correctly-reported TRUE))
	=>
	(modify ?m (desired-lights YELLOW-ON GREEN-BLINK))
)

(defrule machine-lights-broken
	(gamestate (phase PRODUCTION))
	?m <- (machine (name ?n) (state BROKEN) (desired-lights $?dl&:(neq ?dl (create$ RED-BLINK YELLOW-BLINK))))
	(exploration-report (name ?n) (correctly-reported TRUE))
	=>
	(modify ?m (desired-lights RED-BLINK YELLOW-BLINK))
)
