
;---------------------------------------------------------------------------
;  facts.clp - LLSF RefBox CLIPS - facts specification
;
;  Created: Mon Feb 11 13:11:45 2013
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;             2017       Tobias Neumann
;             2019       Till Hofmann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate machine
  (slot name (type SYMBOL)
	(allowed-values C-BS C-DS C-RS1 C-RS2 C-CS1 C-CS2 C-SS M-BS M-DS M-RS1 M-RS2 M-CS1 M-CS2 M-SS))
  (slot team (type SYMBOL) (allowed-values CYAN MAGENTA))
  (slot mtype (type SYMBOL) (allowed-values BS DS RS CS SS))
  (multislot actual-lights (type SYMBOL)
	     (allowed-values RED-ON RED-BLINK YELLOW-ON YELLOW-BLINK GREEN-ON GREEN-BLINK)
	     (default) (cardinality 0 3))
  (multislot desired-lights (type SYMBOL)
	     (allowed-values RED-ON RED-BLINK YELLOW-ON YELLOW-BLINK GREEN-ON GREEN-BLINK)
	     (default GREEN-ON YELLOW-ON RED-ON) (cardinality 0 3))
  (slot productions (type INTEGER) (default 0))
  ; Overall refbox machine state
  (slot state (type SYMBOL) (allowed-values IDLE BROKEN PREPARED PROCESSING
					    PROCESSED READY-AT-OUTPUT DOWN WAIT-IDLE))
  (slot prev-state (type SYMBOL) (default IDLE))
	; The task currently being executed on the MPS
	(slot task (type SYMBOL))
	(slot mps-busy (type SYMBOL) (allowed-values TRUE FALSE WAIT) (default FALSE))
	(slot mps-ready (type SYMBOL) (allowed-values TRUE FALSE WAIT) (default FALSE))
  (slot proc-time (type INTEGER))
  (slot proc-start (type FLOAT))
  (multislot down-period (type FLOAT) (cardinality 2 2) (default -1.0 -1.0))
  (slot broken-since (type FLOAT) (default 0.0))
  (slot broken-reason (type STRING))
   ; x y theta (meters and rad)
  (multislot pose (type FLOAT) (cardinality 3 3) (default 0.0 0.0 0.0))
  (multislot pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (slot zone (type SYMBOL) (default TBD)
	  (allowed-values TBD
      C_Z18 C_Z28 C_Z38 C_Z48 C_Z58 C_Z68 C_Z78 
      C_Z17 C_Z27 C_Z37 C_Z47 C_Z57 C_Z67 C_Z77 
      C_Z16 C_Z26 C_Z36 C_Z46 C_Z56 C_Z66 C_Z76 
      C_Z15 C_Z25 C_Z35 C_Z45 C_Z55 C_Z65 C_Z75 
      C_Z14 C_Z24 C_Z34 C_Z44 C_Z54 C_Z64 C_Z74 
      C_Z13 C_Z23 C_Z33 C_Z43 C_Z53 C_Z63 C_Z73 
      C_Z12 C_Z22 C_Z32 C_Z42 C_Z52 C_Z62 C_Z72 
      C_Z11 C_Z21 C_Z31 C_Z41
      M_Z18 M_Z28 M_Z38 M_Z48 M_Z58 M_Z68 M_Z78 
      M_Z17 M_Z27 M_Z37 M_Z47 M_Z57 M_Z67 M_Z77 
      M_Z16 M_Z26 M_Z36 M_Z46 M_Z56 M_Z66 M_Z76 
      M_Z15 M_Z25 M_Z35 M_Z45 M_Z55 M_Z65 M_Z75 
      M_Z14 M_Z24 M_Z34 M_Z44 M_Z54 M_Z64 M_Z74 
      M_Z13 M_Z23 M_Z33 M_Z43 M_Z53 M_Z63 M_Z73 
      M_Z12 M_Z22 M_Z32 M_Z42 M_Z52 M_Z62 M_Z72 
      M_Z11 M_Z21 M_Z31 M_Z41
    )
  )
  (slot rotation (type INTEGER) (default -1))

  (slot prep-blink-start (type FLOAT))
  (slot idle-since (type FLOAT))
  (slot wait-for-product-since (type FLOAT))
  (slot mps-base-counter (type INTEGER) (default 0))
  (slot bases-added (type INTEGER) (default 0))
  (slot bases-used (type INTEGER) (default 0))

  ; machine type specific slots
  (slot bs-side (type SYMBOL) (allowed-values INPUT OUTPUT))
  (slot bs-color (type SYMBOL) (allowed-values BASE_RED BASE_BLACK BASE_SILVER))

  (slot ds-gate (type INTEGER))
  (slot ds-last-gate (type INTEGER))
  (slot ds-order (type INTEGER))

  (slot ss-operation (type SYMBOL) (allowed-values STORE RETRIEVE))
  (multislot ss-slot (type INTEGER) (cardinality 3 3)) ; meaning defined in llsf_msgs.SSSlot

  (slot rs-ring-color (type SYMBOL)
	(allowed-values RING_BLUE RING_GREEN RING_ORANGE RING_YELLOW))
  (multislot rs-ring-colors (type SYMBOL) (default RING_GREEN RING_BLUE)
	     (allowed-values RING_BLUE RING_GREEN RING_ORANGE RING_YELLOW))

  (slot cs-operation (type SYMBOL) (allowed-values RETRIEVE_CAP MOUNT_CAP))
  (slot cs-retrieved (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
)

(deftemplate machine-mps-state
  (slot name (type SYMBOL))
  (slot state (type SYMBOL))
  (slot num-bases (type INTEGER) (default 0))
)

(deftemplate machine-ss-filled
  (slot name (type SYMBOL))
  (multislot slot (type INTEGER) (cardinality 3 3)) ; meaning defined in llsf_msgs.SSSlot
)

(deftemplate machine-light-code
  (slot id (type INTEGER))
  (multislot code (type SYMBOL) (default)
	     (allowed-values RED-ON RED-BLINK YELLOW-ON YELLOW-BLINK GREEN-ON GREEN-BLINK))
)

(deftemplate machine-generation
  (slot state (type SYMBOL) (default NOT-STARTED)
    (allowed-values NOT-STARTED STARTED FINISHED ABORTED))
  (slot generation-state-last-checked (type FLOAT))
)

(deftemplate mirror-orientation
  (slot cyan (type INTEGER))
  (slot magenta (type INTEGER))
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
  (slot has-pose (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
  (multislot pose (type FLOAT) (cardinality 3 3) (default 0.0 0.0 0.0))
  (multislot pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (multislot vision-pose (type FLOAT) (cardinality 3 3) (default 0.0 0.0 0.0))
  (multislot vision-pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (slot maintenance-start-time (type FLOAT))
  (slot maintenance-cycles (type INTEGER) (default 0))
  (slot maintenance-warning-sent (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
)

(deftemplate robot-beacon
  (multislot rcvd-at (type INTEGER) (cardinality 2 2))

  (slot seq (type INTEGER))
  (multislot time (type INTEGER) (cardinality 2 2))

  (slot number (type INTEGER))
  (slot team-name (type STRING))
  (slot team-color (type SYMBOL) (allowed-values nil CYAN MAGENTA))
  (slot peer-name (type STRING))
  (slot host (type STRING))
  (slot port (type INTEGER))
  (slot has-pose (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
  (multislot pose (type FLOAT) (cardinality 3 3) (default 0.0 0.0 0.0))
  (multislot pose-time (type INTEGER) (cardinality 2 2) (default 0 0))
)

(deftemplate signal
  (slot type)
  (multislot time (type INTEGER) (cardinality 2 2) (default (create$ 0 0)))
  (slot seq (type INTEGER) (default 1))
  (slot count (type INTEGER) (default 1))
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

  ; Product specification
  (slot complexity (type SYMBOL) (allowed-values C0 C1 C2 C3))
  (slot competitive (type SYMBOL) (allowed-values FALSE TRUE) (default FALSE))
  ; the following is auto-generated based on the previously defined complexity
  (slot base-color (type SYMBOL) (allowed-values BASE_RED BASE_SILVER BASE_BLACK))
  (multislot ring-colors (type SYMBOL) (cardinality 0 3)
	     (allowed-values RING_BLUE RING_GREEN RING_ORANGE RING_YELLOW))
  (slot cap-color (type SYMBOL) (allowed-values CAP_BLACK CAP_GREY))

  (slot quantity-requested (type INTEGER) (default 1))
  ; Quantity delivered for both teams in the order C,M
  (multislot quantity-delivered (type INTEGER) (cardinality 2 2) (default 0 0))
  ; Time window in which the order should start, used for
  ; randomizing delivery-period's first element (start time)
  (multislot start-range (type INTEGER) (cardinality 2 2))
  ; Time the production should take, used for randomizing the second
  ; element of delivery-period (end time)
  (multislot duration-range (type INTEGER) (cardinality 2 2) (default 60 180))
  ; Time window in which it must be delivered, set during initial randomization
  (multislot delivery-period (type INTEGER) (cardinality 2 2) (default 0 900))
  (slot delivery-gate (type INTEGER) (default 1))
  (slot active (type SYMBOL) (allowed-values FALSE TRUE) (default FALSE))
  (slot activate-at (type INTEGER) (default 0))
  (multislot activation-range (type INTEGER) (cardinality 2 2) (default 120 240))
  (slot allow-overtime (type SYMBOL) (allowed-values FALSE TRUE) (default FALSE))
)

(deftemplate workpiece
	(slot id (type INTEGER))
	(slot order (type INTEGER))
	(slot rtype (type SYMBOL) (allowed-values INCOMING RECORD))
	(slot at-machine (type SYMBOL)
				(allowed-values C-BS C-DS C-RS1 C-RS2 C-CS1 C-CS2 M-BS M-DS M-RS1 M-RS2 M-CS1 M-CS2))
  (slot state (type SYMBOL) (allowed-values IDLE AVAILABLE RETRIEVED) (default IDLE))
  (slot base-color (type SYMBOL) (allowed-values nil BASE_RED BASE_SILVER BASE_BLACK BASE_CLEAR))
  (multislot ring-colors (type SYMBOL) (cardinality 0 3)
						 (allowed-values RING_BLUE RING_GREEN RING_ORANGE RING_YELLOW))
  (slot cap-color (type SYMBOL) (allowed-values nil CAP_BLACK CAP_GREY))
  (slot team (type SYMBOL) (allowed-values nil CYAN MAGENTA))
  (slot visible (type SYMBOL) (allowed-values FALSE TRUE) (default FALSE))
)

(deftemplate ring-spec
  (slot color (type SYMBOL) (allowed-values RING_BLUE RING_GREEN RING_ORANGE RING_YELLOW))
  (slot req-bases (type INTEGER) (default 0))
)

(deftemplate delivery-period
  (multislot delivery-gates (type SYMBOL) (allowed-values D1 D2 D3 D4 D5 D6) (cardinality 2 2))
  (multislot period (type INTEGER) (cardinality 2 2))
)  
 
(deffunction gen-int-id ()
  "Generate a unique uint that can be used as an ID."
  (bind ?id-string (str-cat (gensym*)))
  (return (string-to-field (sub-string 4 (length$ ?id-string) ?id-string)))
)

(deftemplate referee-confirmation
  (slot process-id (type INTEGER) (default-dynamic (gen-int-id)))
  (slot state (type SYMBOL) (allowed-values REQUIRED CONFIRMED DENIED)
        (default REQUIRED))
)

(deftemplate product-processed
  (slot id (type INTEGER) (default-dynamic (gen-int-id)))
  (slot workpiece (type INTEGER) (default 0))
  (slot game-time (type FLOAT))
  (slot team (type SYMBOL) (allowed-values nil CYAN MAGENTA))
  (slot mtype (type SYMBOL) (allowed-values BS DS RS CS SS))
  (slot at-machine (type SYMBOL)
				(allowed-values C-BS C-DS C-RS1 C-RS2 C-CS1 C-CS2 M-BS M-DS M-RS1 M-RS2 M-CS1 M-CS2))
	(slot delivery-gate (type INTEGER))
  (slot confirmed (type SYMBOL) (allowed-values FALSE TRUE) (default FALSE))
  (slot base-color (type SYMBOL) (allowed-values nil BASE_RED BASE_SILVER BASE_BLACK))
  (slot ring-color (type SYMBOL) (allowed-values nil RING_BLUE RING_GREEN RING_ORANGE RING_YELLOW))
  (slot cap-color (type SYMBOL) (allowed-values nil CAP_BLACK CAP_GREY))
  (slot scored (type SYMBOL) (allowed-values FALSE TRUE) (default FALSE))
	(slot order (type INTEGER) (default 0))
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
	(slot rtype (type SYMBOL) (allowed-values INCOMING RECORD))
  (slot name (type SYMBOL)
	(allowed-values C-BS C-DS C-RS1 C-RS2 C-CS1 C-CS2 M-BS M-DS M-RS1 M-RS2 M-CS1 M-CS2))
  (slot team (type SYMBOL) (allowed-values CYAN MAGENTA))
  (slot zone (type SYMBOL)
	  (allowed-values NOT-REPORTED 
      C_Z18 C_Z28 C_Z38 C_Z48 C_Z58 C_Z68 C_Z78 
      C_Z17 C_Z27 C_Z37 C_Z47 C_Z57 C_Z67 C_Z77 
      C_Z16 C_Z26 C_Z36 C_Z46 C_Z56 C_Z66 C_Z76 
      C_Z15 C_Z25 C_Z35 C_Z45 C_Z55 C_Z65 C_Z75 
      C_Z14 C_Z24 C_Z34 C_Z44 C_Z54 C_Z64 C_Z74 
      C_Z13 C_Z23 C_Z33 C_Z43 C_Z53 C_Z63 C_Z73 
      C_Z12 C_Z22 C_Z32 C_Z42 C_Z52 C_Z62 C_Z72 
      C_Z11 C_Z21 C_Z31 C_Z41
      M_Z18 M_Z28 M_Z38 M_Z48 M_Z58 M_Z68 M_Z78 
      M_Z17 M_Z27 M_Z37 M_Z47 M_Z57 M_Z67 M_Z77 
      M_Z16 M_Z26 M_Z36 M_Z46 M_Z56 M_Z66 M_Z76 
      M_Z15 M_Z25 M_Z35 M_Z45 M_Z55 M_Z65 M_Z75 
      M_Z14 M_Z24 M_Z34 M_Z44 M_Z54 M_Z64 M_Z74 
      M_Z13 M_Z23 M_Z33 M_Z43 M_Z53 M_Z63 M_Z73 
      M_Z12 M_Z22 M_Z32 M_Z42 M_Z52 M_Z62 M_Z72 
      M_Z11 M_Z21 M_Z31 M_Z41
	  )
  )
  (slot rotation (type INTEGER) (default -1))
  (slot host (type STRING))
  (slot port (type INTEGER))
  (slot game-time (type FLOAT))
  (slot correctly-reported (type SYMBOL) (allowed-values UNKNOWN TRUE FALSE) (default UNKNOWN))
  (slot zone-state (type SYMBOL) (allowed-values NO_REPORT CORRECT_REPORT WRONG_REPORT) (default NO_REPORT))
  (slot rotation-state (type SYMBOL) (allowed-values NO_REPORT CORRECT_REPORT WRONG_REPORT) (default NO_REPORT))
)

(deftemplate points
  (slot points (type INTEGER))
  (slot team (type SYMBOL) (allowed-values CYAN MAGENTA))
  (slot game-time (type FLOAT))
  (slot phase (type SYMBOL) (allowed-values EXPLORATION PRODUCTION WHACK_A_MOLE_CHALLENGE))
  (slot reason (type STRING))
  (slot product-step (type INTEGER) (default 0))
)

(deftemplate zone-swap
	(slot m1-name (type SYMBOL))
	(slot m1-new-zone (type SYMBOL))
	(slot m2-name (type SYMBOL))
	(slot m2-new-zone (type SYMBOL))
)

(deftemplate sim-time
  (slot enabled (type SYMBOL) (allowed-values false true) (default false))
  (slot estimate (type SYMBOL) (allowed-values false true) (default false))
  (multislot now (type INTEGER) (cardinality 2 2) (default 0 0))
  (multislot last-recv-time (type INTEGER) (cardinality 2 2) (default 0 0))
  (slot real-time-factor (type FLOAT) (default 0.0))
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
  (machine-generation (state NOT-STARTED))
  (signal (type beacon) (time (create$ 0 0)) (seq 1))
  (signal (type gamestate) (time (create$ 0 0)) (seq 1))
  (signal (type robot-info) (time (create$ 0 0)) (seq 1))
  (signal (type bc-robot-info) (time (create$ 0 0)) (seq 1))
  (signal (type machine-info) (time (create$ 0 0)) (seq 1))
  (signal (type machine-info-bc) (time (create$ 0 0)) (seq 1))
  (signal (type ring-info-bc) (time (create$ 0 0)) (seq 1))
  (signal (type order-info) (time (create$ 0 0)) (seq 1))
  (signal (type machine-report-info) (time (create$ 0 0)) (seq 1))
  (signal (type version-info) (time (create$ 0 0)) (seq 1))
  (signal (type workpiece-info) (time (create$ 0 0)) (seq 1))
  (signal (type setup-light-toggle) (time (create$ 0 0)) (seq 1))
  (setup-light-toggle CS2)
  (whac-a-mole-light NONE)

  (machine (name C-BS)  (team CYAN) (mtype BS)) 
  (machine (name C-DS)  (team CYAN) (mtype DS))
  (machine (name C-SS)  (team CYAN) (mtype SS))
  (machine (name C-RS1) (team CYAN) (mtype RS))
  (machine (name C-RS2) (team CYAN) (mtype RS))
  (machine (name C-CS1) (team CYAN) (mtype CS))
  (machine (name C-CS2) (team CYAN) (mtype CS))

  (machine (name M-BS)  (team MAGENTA) (mtype BS))
  (machine (name M-DS)  (team MAGENTA) (mtype DS))
  (machine (name M-SS)  (team MAGENTA) (mtype SS))
  (machine (name M-RS1) (team MAGENTA) (mtype RS))
  (machine (name M-RS2) (team MAGENTA) (mtype RS))
  (machine (name M-CS1) (team MAGENTA) (mtype CS))
  (machine (name M-CS2) (team MAGENTA) (mtype CS))

  (ring-spec (color RING_BLUE))
  (ring-spec (color RING_GREEN) (req-bases 1))
  (ring-spec (color RING_ORANGE))
  (ring-spec (color RING_YELLOW))

  (mirror-orientation (cyan 0) (magenta 180))
  (mirror-orientation (cyan 45) (magenta 135))
  (mirror-orientation (cyan 90) (magenta 90))
  (mirror-orientation (cyan 135) (magenta 45))
  (mirror-orientation (cyan 180) (magenta 0))
  (mirror-orientation (cyan 225) (magenta 315))
  (mirror-orientation (cyan 270) (magenta 270))
  (mirror-orientation (cyan 315) (magenta 225))

  (machine-ss-filled (name C-SS) (slot 0 0 0))
  (machine-ss-filled (name C-SS) (slot 1 0 0))
  (machine-ss-filled (name C-SS) (slot 2 0 0))
  (machine-ss-filled (name C-SS) (slot 3 0 0))
  (machine-ss-filled (name C-SS) (slot 4 0 0))
  (machine-ss-filled (name C-SS) (slot 5 0 0))
  (machine-ss-filled (name M-SS) (slot 0 0 0))
  (machine-ss-filled (name M-SS) (slot 1 0 0))
  (machine-ss-filled (name M-SS) (slot 2 0 0))
  (machine-ss-filled (name M-SS) (slot 3 0 0))
  (machine-ss-filled (name M-SS) (slot 4 0 0))
  (machine-ss-filled (name M-SS) (slot 5 0 0))
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

; check workpiece-assign-order rule in workpieces.clp for specific
; assumptions for the 2016 game and order to workpiece assignment!
; Especially: single C1, C2, and C3 orders!
(deffacts orders
  ; standing order
  (order (id  1) (complexity C0) (quantity-requested 1) (start-range 0 0)
	       (activation-range 1020 1020) (duration-range 1020 1020))
  (order (id  2) (complexity C1) (quantity-requested 1) (start-range 0 0)
	       (activation-range 1020 1020) (duration-range 1020 1020))
  ; early orders
  (order (id  3) (complexity C2) (quantity-requested 1) (start-range 650 850)
	       (activation-range 500 900) (duration-range 100 200))
  (order (id  4) (complexity C3) (quantity-requested 1) (start-range 650 850)
	       (activation-range 1020 1020) (duration-range 150 200))
  ; normal orders
  (order (id  5) (complexity C0) (quantity-requested 1) (start-range 200 450))
  (order (id  6) (complexity C0) (quantity-requested 2) (start-range 350 800))
  (order (id  7) (complexity C0) (quantity-requested 1) (start-range 800 1020))
  (order (id  8) (complexity C1) (quantity-requested 1) (start-range 550 800)
	       (activation-range 350 550) (duration-range 100 200))
  ; overtime
  (order (id  9) (complexity C0) (competitive TRUE) (quantity-requested 1)
         (start-range 1020 1020) (activation-range 0 0) (duration-range 300 300)
         (allow-overtime TRUE))
)

