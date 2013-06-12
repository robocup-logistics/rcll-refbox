
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
  ?*BC-ORDERINFO-PERIOD* = 5.0
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
  ?*ORDER-QUANTITY-MIN* =  3
  ?*ORDER-QUANTITY-MAX* = 10
  ?*LATE-ORDER-TIME* = 120
  ?*LATE-ORDER-ACTIVATION-PRE-TIME* = 5
  ; number of points for specific actoins
  ?*RECYCLE-POINTS* = 3
  ?*EXPLORATION-CORRECT-REPORT-POINTS* = 4
  ?*EXPLORATION-WRONG-REPORT-POINTS* = -3
  ; number of allowed robot maintenance cycles
  ?*MAINTENANCE-ALLOWED-CYCLES* = 1
  ?*MAINTENANCE-ALLOWED-TIME*   = 120
  ?*MAINTENANCE-WARN-TIME*      = 105
  ?*MAINTENANCE-GRACE-TIME*     =  15
  ; Game phase time; seconds
  ?*SETUP-TIME*       = 300
  ?*EXPLORATION-TIME* = 180
  ?*PRODUCTION-TIME*  = 900
  ; Machine distribution
  ?*RANDOMIZE-GAME* = TRUE
  ?*MACHINE-DISTRIBUTION* = (create$ T1 T1 T1 T1 T2 T2 T2 T3 T4 T5)
)
