
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
  ?*ROBOTINFO-PERIOD* = 2.0
  ?*PUCKINFO-PERIOD* = 2.0
  ?*BC-ORDERINFO-PERIOD* = 5.0
  ?*BC-ORDERINFO-BURST-PERIOD* = 0.5
  ?*BC-MACHINE-REPORT-INFO-PERIOD* = 1.0
  ; number of burst updates before falling back to slower updates
  ?*BC-ORDERINFO-BURST-COUNT* = 10
  ; Machine processing times; seconds
  ?*RECYCLE-PROC-TIME* = 2
  ?*DELIVER-PROC-TIME* = 1
  ?*INTERMEDIATE-PROC-TIME* = 2
  ; number of points for specific actoins
  ?*RECYCLE-POINTS* = 3
  ?*EXPLORATION-CORRECT-REPORT-POINTS* = 4
  ?*EXPLORATION-WRONG-REPORT-POINTS* = -2
  ; Game phase time; seconds
  ?*EXPLORATION-TIME* = 180
  ?*PRODUCTION-TIME* = 900
)
