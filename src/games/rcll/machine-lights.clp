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

(defrule machine-lights-idle
	(gamestate (phase PRODUCTION))
	?m <- (machine (state IDLE) (desired-lights $?dl&:(neq ?dl (create$ GREEN-ON))))
	=>
	(modify ?m (desired-lights GREEN-ON))
)

(defrule machine-lights-down
	(gamestate (phase PRODUCTION))
	?m <- (machine (state DOWN) (desired-lights $?dl&:(neq ?dl (create$ RED-ON))))
	=>
	(modify ?m (desired-lights RED-ON))
)

(defrule machine-lights-prepared-stop-blinking
  "The machine is PREPARED and has been blinking, change the light signal to GREEN (non-blinking)"
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state PREPARED|PROCESSING)
		 (actual-lights GREEN-BLINK) (desired-lights GREEN-BLINK)
		 (prep-blink-start ?bs&:(timeout-sec ?gt ?bs ?*PREPARED-BLINK-TIME*)))
  =>
  (modify ?m (desired-lights GREEN-ON))
)

(defrule machine-lights-processing
	(gamestate (phase PRODUCTION))
	?m <- (machine (state PROCESSING) (desired-lights $?dl&:(neq ?dl (create$ GREEN-ON YELLOW-ON))))
	=>
	(modify ?m (desired-lights GREEN-ON YELLOW-ON))
)

(defrule machine-lights-prepared
	(gamestate (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (state PREPARED) (desired-lights $?dl&:(neq ?dl (create$ GREEN-BLINK))))
	=>
	(modify ?m (desired-lights GREEN-BLINK) (prep-blink-start ?gt))
)

(defrule machine-lights-ready-at-output
  (gamestate (phase PRODUCTION))
  ?m <- (machine (state READY-AT-OUTPUT) (desired-lights $?dl&:(neq ?dl (create$ YELLOW-ON))))
  =>
  (modify ?m (desired-lights YELLOW-ON))
)

(defrule machine-lights-broken
  (gamestate (phase PRODUCTION))
  ?m <- (machine (state BROKEN) (desired-lights $?dl&:(neq ?dl (create$ RED-BLINK YELLOW-BLINK))))
  =>
  (modify ?m (desired-lights RED-BLINK YELLOW-BLINK))
)
