
;---------------------------------------------------------------------------
;  facts.clp - LLSF RefBox CLIPS - facts specification
;
;  Created: Mon Feb 11 13:11:45 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate machine
  (slot name (type SYMBOL) (allowed-values M1 M2 M3 M4 M5 M6 M7 M8 M9 M10 D1 D2 D3 TEST R1 R2))
  (slot mtype (type SYMBOL) (allowed-values T1 T2 T3 T4 T5 DELIVER TEST RECYCLE))
  (multislot loaded-with (type SYMBOL) (allowed-symbols S0 S1 S2))
  (slot junk (type INTEGER) (default 0))
  (slot productions (type INTEGER) (default 0))
  (slot state (type SYMBOL) (allowed-values REQINIT IDLE PROCESSING WAITING DOWN INVALID)
	(default REQINIT))
  (slot proc-time (type INTEGER))
  (multislot proc-start (type INTEGER) (cardinality 2 2) (default (create$ 0 0)))
  (slot puck-id (type INTEGER))
)

(deftemplate puck
  (slot index (type INTEGER))
  (slot id (type INTEGER))
  (slot state (type SYMBOL) (allowed-values S0 S1 S2 P1 P2 P3 JUNK CONSUMED) (default S0))
)

(deftemplate robot
  (slot team (type STRING))
  (slot name (type STRING))
  (slot host (type STRING))
  (slot port (type INTEGER))
  (multislot position (type FLOAT) (cardinality 3 3)) ; x y theta
  (multislot last-seen (type INTEGER) (cardinality 2 2))
)

(deftemplate signal
  (slot type)
  (multislot time (type INTEGER) (cardinality 2 2) (default (create$ 0 0)))
  (slot seq (type INTEGER) (default 1))
)

(deftemplate rfid-input
  (slot machine (type SYMBOL) (allowed-values M1 M2 M3 M4 M5 M6 M7 M8 M9 M10 D1 D2 D3 TEST R1 R2))
  (slot has-puck (type SYMBOL))
  (slot id (type INTEGER))
)

(deftemplate network-client
  (slot id (type INTEGER))
  (slot host (type STRING))
  (slot port (type INTEGER))
)

(deffacts startup
  (time 0 0)
  (state WAIT_START)
  (signal (type beacon) (time (create$ 0 0)) (seq 1))
  (machine (name M1) (mtype T1))
  (machine (name M2) (mtype T1))
  (machine (name M3) (mtype T2))
  (machine (name M4) (mtype T2))
  (machine (name M5) (mtype T3))
  (machine (name M6) (mtype T3))
  (machine (name M7) (mtype T4))
  (machine (name M8) (mtype T4))
  (machine (name M9) (mtype T5))
  (machine (name M10) (mtype T5))
  (machine (name D1) (mtype DELIVER))
  (machine (name D2) (mtype DELIVER))
  (machine (name D3) (mtype DELIVER))
  (machine (name TEST) (mtype TEST))
  (machine (name R1) (mtype RECYCLE))
  (machine (name R2) (mtype RECYCLE))
  (puck (index  1) (id  1))
  (puck (index  2) (id  2))
  (puck (index  3) (id  3))
  (puck (index  4) (id  4))
  (puck (index  5) (id  5))
  (puck (index  6) (id  6))
  (puck (index  7) (id  7))
  (puck (index  8) (id  8))
  (puck (index  9) (id  9))
  (puck (index 10) (id 10))
  (puck (index 11) (id 11))
  (puck (index 12) (id 12))
  (puck (index 13) (id 13))
  (puck (index 14) (id 14))
  (puck (index 15) (id 15))
  (puck (index 16) (id 16))
  (puck (index 17) (id 17))
  (puck (index 18) (id 18))
  (puck (index 19) (id 19))
  (puck (index 20) (id 20))
)
