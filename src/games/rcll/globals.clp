
;---------------------------------------------------------------------------
;  globals.clp - LLSF RefBox global CLIPS variables
;
;  Created: Tue Feb 12 23:26:48 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defglobal
  ; network sending periods; seconds
  ?*BEACON-PERIOD* = 1.0
  ?*GAMESTATE-PERIOD* = 1.0
  ?*ROBOTINFO-PERIOD* = 0.25
  ?*BC-ROBOTINFO-PERIOD* = 2.5
  ?*PUCKINFO-PERIOD* = 1.0
  ?*MACHINE-INFO-PERIOD* = 0.25
  ?*BC-ORDERINFO-PERIOD* = 2.0
  ?*BC-ORDERINFO-BURST-PERIOD* = 0.5
  ?*BC-MACHINE-REPORT-INFO-PERIOD* = 1.0
  ?*BC-EXPLORATION-INFO-PERIOD* = 1.0
  ?*BC-MACHINE-INFO-PERIOD* = 2.0
  ?*BC-MACHINE-INFO-BURST-COUNT* = 30
  ?*BC-MACHINE-INFO-BURST-PERIOD* = 0.5
  ?*BC-RING-INFO-PERIOD* = 2.0
  ?*SYNC-RECONNECT-PERIOD* = 2.0
  ; This value is set by the rule config-timer-interval from config.yaml
  ?*TIMER-INTERVAL* = 0.0
  ; Time (sec) after which to warn about a robot lost
  ?*PEER-LOST-TIMEOUT* = 5
  ?*PEER-REMOVE-TIMEOUT* = 1080
  ?*PEER-TIME-DIFFERENCE-WARNING* = 3.0
  ; number of burst updates before falling back to slower updates
  ?*BC-ORDERINFO-BURST-COUNT* = 10
  ; How often and in what period should the version information
  ; be send over the network when a new peer is detected?
  ?*BC-VERSIONINFO-PERIOD* = 0.5
  ?*BC-VERSIONINFO-COUNT* = 10
  ; Minimum and maximum machine down times, actual value will be
  ; chosen randomly from this range
  ?*DOWN-TIME-MIN* =  30 ;  30
  ?*DOWN-TIME-MAX* = 120 ; 120
  ?*DOWN-TYPES*    = (create$ RS CS)
  ?*BROKEN-DOWN-TIME* = 30
  ?*LOADED-WITH-MAX* = 3
  ; Machine processing times; seconds
  ?*PREPARED-BLINK-TIME* = 3
  ; How long to wait after retrieval to switch to IDLE state
  ?*RETRIEVE-WAIT-IDLE-TIME* = 5

  ; number of points for specific actions
  ?*EXPLORATION-CORRECT-REPORT-TYPE-POINTS* = 5
  ?*EXPLORATION-CORRECT-REPORT-ZONE-POINTS* = 3
  ?*EXPLORATION-WRONG-REPORT-TYPE-POINTS* = -4
  ?*EXPLORATION-WRONG-REPORT-ZONE-POINTS* = -2
  ?*EXPLORATION-INVALID-REPORT-POINTS* = -6
  ?*PRODUCTION-WRONG-TEAM-MACHINE-POINTS* = -2
	?*PRODUCTION-POINTS-ADDITIONAL-BASE* =  2
	?*PRODUCTION-POINTS-FINISH-CC0-STEP* =  5
	?*PRODUCTION-POINTS-FINISH-CC1-STEP* = 10
	?*PRODUCTION-POINTS-FINISH-CC2-STEP* = 20
	?*PRODUCTION-POINTS-FINISH-C1-PRECAP* = 10
	?*PRODUCTION-POINTS-FINISH-C2-PRECAP* = 30
	?*PRODUCTION-POINTS-FINISH-C3-PRECAP* = 80
	?*PRODUCTION-POINTS-MOUNT-CAP* = 10
	?*PRODUCTION-POINTS-DELIVERY*  = 20
  ?*PRODUCTION-POINTS-DELIVERY-TOO-LATE* = 5
  ?*PRODUCTION-POINTS-DELIVERY-WRONG* = 1
  ?*PRODUCTION-DELIVER-MAX-LATENESS-TIME* = 10
  ; Setup light effects
  ?*SETUP-LIGHT-PERIOD* = 1.0
  ?*SETUP-LIGHT-PERIOD-1* = 0.5
  ?*SETUP-LIGHT-PERIOD-2* = 0.25
  ?*SETUP-LIGHT-SPEEDUP-TIME-1* = 240
  ?*SETUP-LIGHT-SPEEDUP-TIME-2* = 270
  ?*SETUP-LIGHT-MACHINES* = (create$ BS DS RS1 RS2 CS1 CS2)
  ; Technical challenge settings
  ?*TECHCHALL-WAM-MACHINES* = (create$ M1 M2 M3 M4 M5 M6 M7 M8 M9 M10)
  ; maximum X or Y distance from signal to accept target reached
  ?*TECHCHALL-WAM-BOX-SIZE* = (create$ 0.65 0.65)
  ?*TECHCHALL-WAM-TIME* = 300
  ?*TECHCHALL-NAVIGATION-TIME* = 60
  ; number of allowed robot maintenance cycles
  ?*MAINTENANCE-ALLOWED-CYCLES* = 1
  ?*MAINTENANCE-ALLOWED-TIME*   = 120
  ?*MAINTENANCE-WARN-TIME*      = 105
  ?*MAINTENANCE-GRACE-TIME*     =  15
  ; Game phase time; seconds
  ?*SETUP-TIME*           = 300
  ?*EXPLORATION-TIME*     = 240
  ?*PRODUCTION-TIME*      = 900
  ?*PRODUCTION-OVERTIME*  = 300
  ; Machine distribution
  ?*RANDOMIZE-GAME* = TRUE
	?*RANDOMIZE-STEPS-MACHINES* = 2
	?*RANDOMIZE-ACTIVATE-ALL-AT-START* = FALSE
	; Incremental randomization probability for switching the machines across
	; field halfs. A value from 0 to 10, 0 no change, 10, always change
	?*RANDOMIZE-INTER-SIDE-SWAP-PROB* = 3
	?*MACHINE-RANDOMIZE-TYPES* = (create$ RS CS)
  ?*MACHINE-ZONES-CYAN* = (create$ Z1 Z2 Z3 Z4 Z5 Z6 Z7 Z8 Z9 Z10 Z11 Z12)
  ?*MACHINE-ZONES-MAGENTA* = (create$ Z13 Z14 Z15 Z16 Z17 Z18 Z19 Z20 Z21 Z22 Z23 Z24)
)
