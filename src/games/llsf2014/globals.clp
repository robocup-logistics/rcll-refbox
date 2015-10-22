
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
  ?*DOWN-NUM-MIN*  =   6
  ?*DOWN-NUM-MAX*  =   8
  ?*RECYCLE-DOWN-TIME-MIN* = 20
  ?*RECYCLE-DOWN-TIME-MAX* = 30
  ; Machine processing times; seconds
  ?*RECYCLE-PROC-TIME* = 2
  ?*DELIVER-PROC-TIME* = 1
  ?*INTERMEDIATE-PROC-TIME* = 2
  ; Delivery gate active times
  ?*DELIVERY-GATE-MIN-TIME* =  60
  ?*DELIVERY-GATE-MAX-TIME* = 180
  ; Grace time for accepted delivery after machine down
  ?*DELIVERY-GATE-GRACE-TIME* = 3
  ; number of products per order
  ?*ORDER-ACTIVATION-PRE-TIME-MIN* = 10
  ?*ORDER-ACTIVATION-PRE-TIME-MAX* = 60
  ?*ORDER-MIN-DELIVER-TIME* = 30
  ?*ORDER-MAX-DOWN-RATIO* = 0.5
  ?*ORDER-DOWN-SHRINK* = 20
  ; number of points for specific actions
  ?*RECYCLE-POINTS* = 5
  ?*EXPLORATION-CORRECT-REPORT-POINTS* = 4
  ?*EXPLORATION-WRONG-REPORT-POINTS* = -3
  ?*EXPLORATION-INVALID-REPORT-POINTS* = -3
  ?*PRODUCTION-WRONG-TEAM-MACHINE-POINTS* = -2
  ?*DELIVER-WITH-NO-ACTIVE-ORDER* = 1
  ?*PRODUCED-IN-DELIVER-TIME-POINTS* = 5
  ; Setup light effects
  ?*SETUP-LIGHT-PERIOD* = 1.0
  ?*SETUP-LIGHT-PERIOD-1* = 0.5
  ?*SETUP-LIGHT-PERIOD-2* = 0.25
  ?*SETUP-LIGHT-SPEEDUP-TIME-1* = 240
  ?*SETUP-LIGHT-SPEEDUP-TIME-2* = 270
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
  ; Machine types that may always be announced
  ?*MACHINE-UNRESTRICTED-TYPES* = (create$ DELIVER RECYCLE)
  ; Machine types that might be swapped in phase 1
  ?*TOURNAMENT-PHASES* = (create$ ROUND-ROBIN PLAY-OFFS FINALS)
  ?*TOURNAMENT-PHASE* = ROUND-ROBIN
  ?*MACHINE-SWAP-ROUND-ROBIN* = (create$ T3 T4 T5)
  ?*MACHINE-SWAP-PLAY-OFFS-NUM* = 6
  ?*MACHINE-SWAP-PLAY-OFFS-TYPES* = (create$ T1 T2 T3 T4 T5)
  ?*MACHINE-SWAP-FINALS-TYPES* = (create$ T1 T2 T3 T4 T5)
  ; Machine distribution
  ?*RANDOMIZE-GAME* = TRUE
  ?*MACHINE-DISTRIBUTION* = (create$ T1 T1 T1 T1 T2 T2 T2 T3 T3 T4 T4 T5)
)
