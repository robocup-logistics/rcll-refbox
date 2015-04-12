
;---------------------------------------------------------------------------
;  facts.clp - LLSF RefBox CLIPS - facts specification
;
;  Created: Mon Feb 11 13:11:45 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate machine
  (slot name (type SYMBOL)
	(allowed-values M1 M2 M3 M4 M5 M6 M7 M8 M9 M10 M11 M12 D1 D2 D3 R1
			M13 M14 M15 M16 M17 M18 M19 M20 M21 M22 M23 M24 D4 D5 D6 R2))
  (slot team (type SYMBOL) (allowed-values CYAN MAGENTA))
  (slot mtype (type SYMBOL) (allowed-values T1 T2 T3 T4 T5 DELIVER RECYCLE))
  (multislot loaded-with (type INTEGER) (default))
  (multislot actual-lights (type SYMBOL)
	     (allowed-values RED-ON RED-BLINK YELLOW-ON YELLOW-BLINK GREEN-ON GREEN-BLINK)
	     (default) (cardinality 0 3))
  (multislot desired-lights (type SYMBOL)
	     (allowed-values RED-ON RED-BLINK YELLOW-ON YELLOW-BLINK GREEN-ON GREEN-BLINK)
	     (default GREEN-ON YELLOW-ON RED-ON) (cardinality 0 3))
  (slot productions (type INTEGER) (default 0))
  (slot state (type SYMBOL) (allowed-values IDLE PROCESSING WAITING DOWN INVALID))
  (slot prev-state (type SYMBOL) (allowed-values IDLE PROCESSING WAITING DOWN INVALID))
  (slot proc-time (type INTEGER))
  (slot proc-start (type FLOAT))
  (multislot down-period (type FLOAT) (cardinality 2 2) (default -1.0 -1.0))
  (slot puck-id (type INTEGER) (default 0))
   ; x y theta (meters and rad)
  (multislot pose (type FLOAT) (cardinality 3 3) (default 0.0 0.0 0.0))
  (multislot pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
)

(deftemplate machine-spec
  (slot mtype (type SYMBOL) (allowed-values T1 T2 T3 T4 T5 DELIVER RECYCLE))
  (multislot inputs (type SYMBOL) (allowed-symbols S0 S1 S2) (default))
  (slot output (type SYMBOL) (allowed-symbols NONE S0 S1 S2 P1 P2 P3))
  (slot proc-time-min (type INTEGER))
  (slot proc-time-max (type INTEGER))
  (slot proc-time (type INTEGER))
  (slot light-code (type INTEGER) (default 0))
  (slot points (type INTEGER) (default 0))
)

(deftemplate machine-light-code
  (slot id (type INTEGER))
  (multislot code (type SYMBOL) (default)
	     (allowed-values RED-ON RED-BLINK YELLOW-ON YELLOW-BLINK GREEN-ON GREEN-BLINK))
)

(deftemplate puck
  (slot index (type INTEGER))
  (slot id (type INTEGER))
  (slot team (type SYMBOL) (allowed-values nil CYAN MAGENTA) (default nil))
  (slot state (type SYMBOL) (allowed-values S0 S1 S2 P1 P2 P3 CONSUMED FINISHED) (default S0))
  (slot state-change-game-time (type FLOAT))
   ; x y theta (meters and rad)
  (multislot pose (type FLOAT) (cardinality 2 2) (default 0.0 0.0))
  (multislot pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
)

(deftemplate robot
  (slot number (type INTEGER))
  (slot state (type SYMBOL) (allowed-values ACTIVE MAINTENANCE DISQUALIFIED) (default ACTIVE))
  (slot team (type STRING))
  (slot team-color (type SYMBOL) (allowed-values nil CYAN MAGENTA))
  (slot name (type STRING))
  (slot host (type STRING))
  (slot port (type INTEGER))
  (multislot last-seen (type INTEGER) (cardinality 2 2))
  (slot warning-sent (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
   ; x y theta (meters and rad)
  (multislot pose (type FLOAT) (cardinality 3 3) (default 0.0 0.0 0.0))
  (multislot pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (multislot vision-pose (type FLOAT) (cardinality 3 3) (default 0.0 0.0 0.0))
  (multislot vision-pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (slot maintenance-start-time (type FLOAT))
  (slot maintenance-cycles (type INTEGER) (default 0))
  (slot maintenance-warning-sent (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
)

(deftemplate signal
  (slot type)
  (multislot time (type INTEGER) (cardinality 2 2) (default (create$ 0 0)))
  (slot seq (type INTEGER) (default 1))
  (slot count (type INTEGER) (default 1))
)

(deftemplate rfid-input
  (slot machine (type SYMBOL)
	(allowed-values M1 M2 M3 M4 M5 M6 M7 M8 M9 M10 M11 M12 D1 D2 D3 R1
			M13 M14 M15 M16 M17 M18 M19 M20 M21 M22 M23 M24 D4 D5 D6 R2))
  (slot has-puck (type SYMBOL))
  (slot id (type INTEGER))
)

(deftemplate network-client
  (slot id (type INTEGER))
  (slot host (type STRING))
  (slot port (type INTEGER))
  (slot is-slave (type SYMBOL) (allowed-values FALSE TRUE) (default FALSE))
)

(deftemplate network-peer
  (slot group (type SYMBOL) (allowed-values PUBLIC CYAN MAGENTA))
  (slot id (type INTEGER))
  (slot network-prefix (type STRING))
)

(deftemplate attention-message
  (slot team (type SYMBOL) (allowed-values nil CYAN MAGENTA) (default nil))
  (slot text (type STRING))
  (slot time (type INTEGER) (default 5))
)

(deftemplate order
  (slot id (type INTEGER))
  (slot team (type SYMBOL) (allowed-values CYAN MAGENTA))
  (slot product (type SYMBOL) (allowed-values P1 P2 P3))
  (slot quantity-requested (type INTEGER) (default 1))
  (slot quantity-delivered (type INTEGER) (default 0))
  ; Time window in which the order should start, used for
  ; randomizing delivery-period's first element (start time)
  (multislot start-range (type INTEGER) (cardinality 2 2))
  ; Time the production should take, used for randomizing the second
  ; element of delivery-period (end time)
  (multislot duration-range (type INTEGER) (cardinality 2 2) (default 30 180))
  ; Time window in which it must be delivered, set during initial randomization
  (multislot delivery-period (type INTEGER) (cardinality 2 2) (default 0 900))
  (slot delivery-gate (type SYMBOL) (allowed-values ANY D1 D2 D3) (default ANY))
  (slot active (type SYMBOL) (allowed-values FALSE TRUE) (default FALSE))
  (slot activate-at (type INTEGER) (default 0))
  (slot points (type INTEGER) (default 10))
  (slot points-supernumerous (type INTEGER) (default 1))
)

(deftemplate delivery-period
  (multislot delivery-gates (type SYMBOL) (allowed-values D1 D2 D3 D4 D5 D6) (cardinality 2 2))
  (multislot period (type INTEGER) (cardinality 2 2))
)  
 
(deftemplate product-delivered
  (slot game-time (type FLOAT))
  (slot production-time (type FLOAT))
  (slot team (type SYMBOL) (allowed-values nil CYAN MAGENTA))
  (slot product (type SYMBOL) (allowed-values P1 P2 P3))
  (slot delivery-gate (type SYMBOL) (allowed-values D1 D2 D3 D4 D5 D6))
)

(deftemplate gamestate
  (slot refbox-mode (type SYMBOL) (allowed-values STANDALONE) (default STANDALONE))
  (slot state (type SYMBOL)
	(allowed-values INIT WAIT_START RUNNING PAUSED) (default INIT))
  (slot prev-state (type SYMBOL)
	(allowed-values INIT WAIT_START RUNNING PAUSED) (default INIT))
  (slot phase (type SYMBOL)
	(allowed-values PRE_GAME SETUP EXPLORATION PRODUCTION POST_GAME
			OPEN_CHALLENGE NAVIGATION_CHALLENGE WHACK_A_MOLE_CHALLENGE)
	(default PRE_GAME))
  (slot prev-phase (type SYMBOL)
	(allowed-values NONE PRE_GAME SETUP EXPLORATION PRODUCTION POST_GAME
			OPEN_CHALLENGE NAVIGATION_CHALLENGE WHACK_A_MOLE_CHALLENGE)
	(default NONE))
  (slot game-time (type FLOAT) (default 0.0))
  (slot cont-time (type FLOAT) (default 0.0))
  ; cardinality 2: sec msec
  (multislot start-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (multislot end-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (multislot last-time (type INTEGER) (cardinality 2 2) (default 0 0))
  ; cardinality 2: team cyan and magenta
  (multislot points (type INTEGER) (cardinality 2 2) (default 0 0))
  (multislot teams (type STRING) (cardinality 2 2) (default "" ""))

  (slot over-time (type SYMBOL) (allowed-values FALSE TRUE) (default FALSE))
)

(deftemplate exploration-report
  (slot name (type SYMBOL)
	(allowed-values M1 M2 M3 M4 M5 M6 M7 M8 M9 M10 M11 M12
			M13 M14 M15 M16 M17 M18 M19 M20 M21 M22 M23 M24))
  (slot team (type SYMBOL) (allowed-values CYAN MAGENTA))
  (slot type (type SYMBOL) (allowed-values WRONG T1 T2 T3 T4 T5))
  (slot host (type STRING))
  (slot port (type INTEGER))
  (slot game-time (type FLOAT))
)

(deftemplate points
  (slot points (type INTEGER))
  (slot team (type SYMBOL) (allowed-values CYAN MAGENTA))
  (slot game-time (type FLOAT))
  (slot phase (type SYMBOL) (allowed-values EXPLORATION PRODUCTION WHACK_A_MOLE_CHALLENGE))
  (slot reason (type STRING))
)

; Machine directions in LLSF arena frame when looking from bird's eye perspective
(defglobal
  ?*M-EAST*   = (* (/ 3.0 2.0) (pi))   ; 270 deg or -90 deg
  ?*M-NORTH*  = 0                      ;   0 deg
  ?*M-WEST*   = (/ (pi) 2.0)           ;  90 deg
  ?*M-SOUTH*  = (pi)                   ; 180 deg
)

(deffacts startup
  (gamestate (phase PRE_GAME))
  (signal (type beacon) (time (create$ 0 0)) (seq 1))
  (signal (type gamestate) (time (create$ 0 0)) (seq 1))
  (signal (type robot-info) (time (create$ 0 0)) (seq 1))
  (signal (type bc-robot-info) (time (create$ 0 0)) (seq 1))
  (signal (type machine-info) (time (create$ 0 0)) (seq 1))
  (signal (type machine-info-bc) (time (create$ 0 0)) (seq 1))
  (signal (type puck-info) (time (create$ 0 0)) (seq 1))
  (signal (type order-info) (time (create$ 0 0)) (seq 1))
  (signal (type machine-report-info) (time (create$ 0 0)) (seq 1))
  (signal (type version-info) (time (create$ 0 0)) (seq 1))
  (signal (type exploration-info) (time (create$ 0 0)) (seq 1))
  (signal (type setup-light-toggle) (time (create$ 0 0)) (seq 1))
  (setup-light-toggle 0 0)
  (whac-a-mole-light NONE)
  ; Positions are the example ones from the rulebook and
  ; will most likely be different during the tournament
  (machine (name M1)  (team CYAN) (mtype T1)      (pose 0.56 1.68 ?*M-EAST*))
  (machine (name M2)  (team CYAN) (mtype T1)      (pose 0.56 2.80 ?*M-WEST*))
  (machine (name M3)  (team CYAN) (mtype T2)      (pose 1.68 1.68 ?*M-NORTH*))
  (machine (name M4)  (team CYAN) (mtype T2)      (pose 1.68 2.80 ?*M-SOUTH*))
  (machine (name M5)  (team CYAN) (mtype T3)      (pose 1.68 3.92 ?*M-SOUTH*))
  (machine (name M6)  (team CYAN) (mtype T3)      (pose 2.80 1.68 ?*M-WEST*))
  (machine (name M7)  (team CYAN) (mtype T4)      (pose 2.80 3.92 ?*M-EAST*))
  (machine (name M8)  (team CYAN) (mtype T5)      (pose 2.80 5.04 ?*M-SOUTH*))
  (machine (name M9)  (team CYAN) (mtype T4)      (pose 3.92 1.68 ?*M-NORTH*))
  (machine (name M10) (team CYAN) (mtype T5)      (pose 3.92 2.80 ?*M-WEST*))
  (machine (name M11) (team CYAN) (mtype T3)      (pose 3.92 5.04 ?*M-EAST*))
  (machine (name M12) (team CYAN) (mtype T4)      (pose 5.04 5.04 ?*M-EAST*))
  (machine (name D1)  (team CYAN) (mtype DELIVER) (pose 5.34 2.45 ?*M-SOUTH*))
  (machine (name D2)  (team CYAN) (mtype DELIVER) (pose 5.34 2.80 ?*M-SOUTH*))
  (machine (name D3)  (team CYAN) (mtype DELIVER) (pose 5.34 3.15 ?*M-SOUTH*))
  (machine (name R1)  (team CYAN) (mtype RECYCLE) (pose 0.56 5.04 ?*M-NORTH*))

  (machine (name M13) (team MAGENTA) (mtype T1)      (pose -0.56 1.68 ?*M-EAST*))
  (machine (name M14) (team MAGENTA) (mtype T1)      (pose -0.56 2.80 ?*M-WEST*))
  (machine (name M15) (team MAGENTA) (mtype T2)      (pose -1.68 1.68 ?*M-SOUTH*))
  (machine (name M16) (team MAGENTA) (mtype T2)      (pose -1.68 2.80 ?*M-NORTH*))
  (machine (name M17) (team MAGENTA) (mtype T3)      (pose -1.68 3.92 ?*M-NORTH*))
  (machine (name M18) (team MAGENTA) (mtype T3)      (pose -2.80 1.68 ?*M-WEST*))
  (machine (name M19) (team MAGENTA) (mtype T4)      (pose -2.80 3.92 ?*M-EAST*))
  (machine (name M20) (team MAGENTA) (mtype T5)      (pose -2.80 5.04 ?*M-NORTH*))
  (machine (name M21) (team MAGENTA) (mtype T4)      (pose -3.92 1.68 ?*M-SOUTH*))
  (machine (name M22) (team MAGENTA) (mtype T5)      (pose -3.92 2.80 ?*M-WEST*))
  (machine (name M23) (team MAGENTA) (mtype T3)      (pose -3.92 5.04 ?*M-EAST*))
  (machine (name M24) (team MAGENTA) (mtype T4)      (pose -5.04 5.04 ?*M-EAST*))
  (machine (name D4)  (team MAGENTA) (mtype DELIVER) (pose -5.34 2.45 ?*M-NORTH*))
  (machine (name D5)  (team MAGENTA) (mtype DELIVER) (pose -5.34 2.80 ?*M-NORTH*))
  (machine (name D6)  (team MAGENTA) (mtype DELIVER) (pose -5.34 3.15 ?*M-NORTH*))
  (machine (name R2)  (team MAGENTA) (mtype RECYCLE) (pose -0.56 5.04 ?*M-SOUTH*))
)

(deffacts light-codes
  (machine-light-code (id 1) (code GREEN-ON))
  (machine-light-code (id 2) (code YELLOW-ON))
  (machine-light-code (id 3) (code GREEN-ON YELLOW-ON))
  (machine-light-code (id 4) (code RED-ON))
  (machine-light-code (id 5) (code RED-ON GREEN-ON))
  (machine-light-code (id 6) (code RED-ON YELLOW-ON))
  (machine-light-code (id 7) (code RED-ON YELLOW-ON GREEN-ON))
  ;(machine-light-code (id 8) (code GREEN-BLINK))
  ;(machine-light-code (id 9) (code YELLOW-BLINK))
  ;(machine-light-code (id 10) (code RED-BLINK))
)

(deffacts machine-specs
  (machine-spec (mtype T1) (inputs S0) (output S1)
		(light-code 1) (points 0)
		(proc-time-min 3) (proc-time-max 8) (proc-time 4)) ; 3 8
  (machine-spec (mtype T2) (inputs S0 S1) (output S2)
		(light-code 2) (points 4)
		(proc-time-min 15) (proc-time-max 25)) ; 15 25
  (machine-spec (mtype T3) (inputs S0 S1 S2) (output P1)
		(light-code 4) (points 12)
		(proc-time-min 40) (proc-time-max 60) (proc-time 4)) ; 40 60
  (machine-spec (mtype T4) (inputs S0 S1 S2) (output P2)
		(light-code 5) (points 12)
		(proc-time-min 40) (proc-time-max 60) (proc-time 4)) ; 40 60
  (machine-spec (mtype T5) (inputs S0) (output P3)
		(light-code 6) (points 0)
		(proc-time-min 20) (proc-time-max 40) (proc-time 4)) ; 20 40
)

(deffacts orders
  ; CYAN
  ; 40 points in P3
  (order (id  1) (team CYAN) (product P3) (quantity-requested 1) (start-range 0 120))
  (order (id  2) (team CYAN) (product P3) (quantity-requested 1) (start-range 100 300))
  (order (id  3) (team CYAN) (product P3) (quantity-requested 2) (start-range 300 700))
  ; 20 points as P1 and P2 each
  (order (id  4) (team CYAN) (product P1) (quantity-requested 1) (start-range 400 520))
  (order (id  5) (team CYAN) (product P2) (quantity-requested 1) (start-range 600 720))

  ; MAGENTA orders will be created automatically after parameterizing CYAN orders
)
