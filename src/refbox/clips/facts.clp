
;---------------------------------------------------------------------------
;  facts.clp - LLSF RefBox CLIPS - facts specification
;
;  Created: Mon Feb 11 13:11:45 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate machine
  (slot name (type SYMBOL) (allowed-values M1 M2 M3 M4 M5 M6 M7 M8 M9 M10 D1 D2 D3 TST R1 R2))
  (slot mtype (type SYMBOL) (allowed-values T1 T2 T3 T4 T5 DELIVER TEST RECYCLE))
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
  (multislot proc-start (type FLOAT))
  (slot down-possible (type SYMBOL) (allowed-values TRUE FALSE) (default TRUE))
  (multislot down-period (type FLOAT) (cardinality 2 2) (default -1.0 -1.0))
  (slot puck-id (type INTEGER) (default 0))
   ; x y theta (meters and rad)
  (multislot pose (type FLOAT) (cardinality 3 3) (default 0.0 0.0 0.0))
  (multislot pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
)

(deftemplate machine-spec
  (slot mtype (type SYMBOL) (allowed-values T1 T2 T3 T4 T5 DELIVER TEST RECYCLE))
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
  (slot state (type SYMBOL) (allowed-values S0 S1 S2 P1 P2 P3 CONSUMED) (default S0))
   ; x y theta (meters and rad)
  (multislot pose (type FLOAT) (cardinality 2 2) (default 0.0 0.0))
  (multislot pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
)

(deftemplate robot
  (slot number (type INTEGER))
  (slot team (type STRING))
  (slot name (type STRING))
  (slot host (type STRING))
  (slot port (type INTEGER))
  (multislot last-seen (type INTEGER) (cardinality 2 2))
  (slot warning-sent (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
   ; x y theta (meters and rad)
  (multislot pose (type FLOAT) (cardinality 3 3) (default 0.0 0.0 0.0))
  (multislot pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
)

(deftemplate signal
  (slot type)
  (multislot time (type INTEGER) (cardinality 2 2) (default (create$ 0 0)))
  (slot seq (type INTEGER) (default 1))
  (slot count (type INTEGER) (default 1))
)

(deftemplate rfid-input
  (slot machine (type SYMBOL) (allowed-values M1 M2 M3 M4 M5 M6 M7 M8 M9 M10 D1 D2 D3 TST R1 R2))
  (slot has-puck (type SYMBOL))
  (slot id (type INTEGER))
)

(deftemplate network-client
  (slot id (type INTEGER))
  (slot host (type STRING))
  (slot port (type INTEGER))
)

(deftemplate order
  (slot id (type INTEGER))
  (slot product (type SYMBOL) (allowed-values P1 P2 P3))
  (slot quantity-requested (type INTEGER) (default 1))
  (slot quantity-delivered (type INTEGER) (default 0))
  (multislot delivery-period (type INTEGER) (cardinality 2 2) (default 0 900))
  (slot delivery-gate (type SYMBOL) (allowed-values ANY D1 D2 D3) (default ANY))
  (slot late-order (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
  (multislot late-order-start-period (type INTEGER) (cardinality 2 2))
  (slot active (type SYMBOL) (allowed-values TRUE FALSE) (default TRUE))
  (slot activate-at (type INTEGER) (default 0))
  (slot points (type INTEGER) (default 10))
  (slot points-supernumerous (type INTEGER) (default 1))
)

(deftemplate delivery-period
  (slot delivery-gate (type SYMBOL) (allowed-values D1 D2 D3))
  (multislot period (type INTEGER) (cardinality 2 2))
)  
 
(deftemplate product-delivered
  (multislot game-time (type FLOAT))
  (slot product (type SYMBOL) (allowed-values P1 P2 P3))
  (slot delivery-gate (type SYMBOL) (allowed-values D1 D2 D3))
)

(deftemplate gamestate
  (slot state (type SYMBOL)
	(allowed-values INIT WAIT_START RUNNING PAUSED) (default INIT))
  (slot prev-state (type SYMBOL)
	(allowed-values INIT WAIT_START RUNNING PAUSED) (default INIT))
  (slot phase (type SYMBOL)
	(allowed-values PRE_GAME EXPLORATION PRODUCTION POST_GAME) (default PRE_GAME))
  (slot prev-phase (type SYMBOL)
	(allowed-values PRE_GAME EXPLORATION PRODUCTION POST_GAME) (default PRE_GAME))
  (slot game-time (type FLOAT) (default 0.0))
  (multislot last-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (slot points (type INTEGER) (default 0))
  (slot team (type STRING))
)

(deftemplate exploration-report
  (slot name (type SYMBOL) (allowed-values M1 M2 M3 M4 M5 M6 M7 M8 M9 M10))
  (slot type (type SYMBOL) (allowed-values WRONG T1 T2 T3 T4 T5))
  (slot host (type STRING))
  (slot port (type INTEGER))
  (slot game-time (type FLOAT))
)

(deftemplate points
  (slot points (type INTEGER))
  (slot game-time (type FLOAT))
  (slot phase (type SYMBOL) (allowed-values EXPLORATION PRODUCTION))
  (slot reason (type STRING))
)

; Machine directions in LLSF arena frame when looking from bird's eye perspective
(defglobal
  ?*M-RIGHT*      = 0
  ?*M-UP*         = (/ (pi) 2.0)
  ?*M-LEFT*       = (pi)
  ?*M-DOWN*       = (* (/ 3.0 2.0) (pi))
  ?*M-UP-RIGHT*   = (* (/ 1.0 4.0) (pi))
  ?*M-UP-LEFT*   =  (* (/ 3.0 4.0) (pi))
  ?*M-DOWN-LEFT*  = (* (/ 5.0 4.0) (pi))
  ?*M-DOWN-RIGHT* = (* (/ 7.0 4.0) (pi))
)

(deffacts startup
  (gamestate (phase PRE_GAME))
  (signal (type beacon) (time (create$ 0 0)) (seq 1))
  (signal (type gamestate) (time (create$ 0 0)) (seq 1))
  (signal (type robot-info) (time (create$ 0 0)) (seq 1))
  (signal (type machine-info) (time (create$ 0 0)) (seq 1))
  (signal (type machine-info-bc) (time (create$ 0 0)) (seq 1))
  (signal (type puck-info) (time (create$ 0 0)) (seq 1))
  (signal (type order-info) (time (create$ 0 0)) (seq 1))
  (signal (type machine-report-info) (time (create$ 0 0)) (seq 1))
  (signal (type version-info) (time (create$ 0 0)) (seq 1))
  (signal (type exploration-info) (time (create$ 0 0)) (seq 1))
  ; Positions are the example ones from the rulebook and
  ; will most likely be different during the tournament
  (machine (name M1)  (mtype T1)      (pose 3.92 1.68 ?*M-DOWN*))
  (machine (name M2)  (mtype T1)      (pose 3.92 3.92 ?*M-RIGHT*))
  (machine (name M3)  (mtype T2)      (pose 2.80 0.56 ?*M-UP*))
  (machine (name M4)  (mtype T2)      (pose 2.80 1.68 ?*M-UP*))
  (machine (name M5)  (mtype T3)      (pose 2.80 2.80 ?*M-LEFT*))
  (machine (name M6)  (mtype T3)      (pose 2.80 3.92 ?*M-UP*))
  (machine (name M7)  (mtype T4)      (pose 2.80 5.04 ?*M-DOWN*))
  (machine (name M8)  (mtype T5)      (pose 1.68 1.68 ?*M-LEFT*))
  (machine (name M9)  (mtype T4)      (pose 1.68 3.92 ?*M-DOWN*))
  (machine (name M10) (mtype T5)      (pose 1.68 5.04 ?*M-RIGHT*))
  (machine (name D1)  (mtype DELIVER) (pose 5.34 3.15 ?*M-LEFT*)      (down-possible FALSE))
  (machine (name D2)  (mtype DELIVER) (pose 5.34 2.80 ?*M-LEFT*)      (down-possible FALSE))
  (machine (name D3)  (mtype DELIVER) (pose 5.34 2.45 ?*M-LEFT*)      (down-possible FALSE))
  (machine (name TST) (mtype TEST)    (pose 5.40 5.40 ?*M-DOWN-LEFT*) (down-possible FALSE))
  (machine (name R1)  (mtype RECYCLE) (pose 5.40 0.20 ?*M-UP-LEFT*))
  (machine (name R2)  (mtype RECYCLE) (pose 0.20 5.40 ?*M-DOWN-RIGHT*))
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
  (order (id 1) (product P1) (quantity-requested 3) (delivery-period   0 299))
  (order (id 2) (product P2) (quantity-requested 2) (delivery-period   0 299))
  (order (id 3) (product P3) (quantity-requested 4) (delivery-period   0 299))
  (order (id 4) (product P1) (quantity-requested 1) (delivery-period 300 599))
  (order (id 5) (product P2) (quantity-requested 6) (delivery-period 300 599))
  (order (id 6) (product P3) (quantity-requested 2) (delivery-period 300 599))
  (order (id 7) (product P1) (quantity-requested 3) (delivery-period 600 900))
  (order (id 8) (product P2) (quantity-requested 3) (delivery-period 600 900))
  (order (id 9) (product P3) (quantity-requested 3) (delivery-period 600 900))
  ; Late orders
  (order (id 10) (product P3) (quantity-requested 1) (points 20) (points-supernumerous 0)
	 (active FALSE) (activate-at 5)
	 (late-order TRUE) (late-order-start-period 120 400))
  (order (id 11) (product P3) (quantity-requested 1) (points 20) (points-supernumerous 0)
	 (active FALSE) (activate-at 395)
	 (late-order TRUE) (late-order-start-period 520 780))
)
