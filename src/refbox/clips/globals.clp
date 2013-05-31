
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
  ?*ROBOTINFO-PERIOD* = 1.0
  ?*PUCKINFO-PERIOD* = 1.0
  ?*MACHINE-INFO-PERIOD* = 0.25
  ?*BC-ORDERINFO-PERIOD* = 5.0
  ?*BC-ORDERINFO-BURST-PERIOD* = 0.5
  ?*BC-MACHINE-REPORT-INFO-PERIOD* = 1.0
  ?*BC-EXPLORATION-INFO-PERIOD* = 1.0
  ?*BC-MACHINE-INFO-PERIOD* = 2.0
  ?*BC-MACHINE-INFO-BURST-COUNT* = 30
  ?*BC-MACHINE-INFO-BURST-PERIOD* = 0.5
  ; Time after which to warn about a robot lost
  ?*PEER-LOST-TIMEOUT* = 5.0
  ?*PEER-REMOVE-TIMEOUT* = 30.0
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
  ?*DELIVERY-GATE-MIN-TIME* = 30
  ?*DELIVERY-GATE-MAX-TIME* = 120
  ; number of points for specific actoins
  ?*RECYCLE-POINTS* = 3
  ?*EXPLORATION-CORRECT-REPORT-POINTS* = 4
  ?*EXPLORATION-WRONG-REPORT-POINTS* = -3
  ; Game phase time; seconds
  ?*EXPLORATION-TIME* = 180
  ?*PRODUCTION-TIME* = 900
  ; Machine distribution
  ?*RANDOMIZE-GAME* = TRUE
  ?*MACHINE-DISTRIBUTION* = (create$ T1 T1 T1 T1 T2 T2 T2 T3 T4 T5)
)
