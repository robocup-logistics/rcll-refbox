
; Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

;---------------------------------------------------------------------------
;  globals.clp - LLSF RefBox global CLIPS variables
;
;  Created: Tue Feb 12 23:26:48 2013
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;             2017       Tobias Neumann
;             2019       Till Hofmann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defglobal
  ; network sending periods; seconds
  ?*BEACON-PERIOD* = 1.0
  ?*GAMESTATE-PERIOD* = 1.0
  ?*ROBOTINFO-PERIOD* = 0.25
  ?*BC-ROBOTINFO-PERIOD* = 2.5
  ?*WORKPIECEINFO-PERIOD* = 2.0
  ?*MACHINE-INFO-PERIOD* = 0.25
  ?*BC-ORDERINFO-PERIOD* = 2.0
  ?*BC-ORDERINFO-BURST-PERIOD* = 0.5
  ?*BC-MACHINE-REPORT-INFO-PERIOD* = 1.0
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
  ?*BC-VERSIONINFO-PERIOD* = 5.0
  ?*BC-VERSIONINFO-COUNT* = 10
  ; Minimum and maximum machine down times, actual value will be
  ; chosen randomly from this range
  ?*DOWN-TIME-MIN* =  30 ;  30
  ?*DOWN-TIME-MAX* = 60 ; 120
  ?*DOWN-TYPES*    = (create$ RS CS)
  ?*BROKEN-DOWN-TIME* = 30
  ?*LOADED-WITH-MAX* = 3
  ; Machine processing times; seconds
  ?*PREPARED-BLINK-TIME* = 3
  ; How long to stay in WAIT-IDLE before switching to IDLE state
  ?*WAIT-IDLE-TIME* = 5
  ; How long to wait after prepare before the MPS gets resetted
  ?*PREPARE-WAIT-TILL-RESET* = 45
  ; How long to wait before resetting a machine that is processing
  ?*PROCESSING-WAIT-TILL-RESET* = 20
  ?*PROCESSING-WAIT-TILL-WARNING* = 10
  ?*PREPARE-WAIT-TILL-PROCESSING* = 5
  ?*PROCESS-TIME-SS* = 5
  ?*SS-MAX-SHELF* = 5 ; min value is 0
  ?*SS-MAX-SLOT* = 7 ; min value is 0
  ?*SS-SHELF-DEPTH* = 2


  ?*MAX-ROBOTS-PER-TEAM* = 3
  ?*NUMBER-OF-ORDERS* = 11
  ?*BEACON-PERIOD* = 1.0

  ; number of points for specific actions
  ?*EXPLORATION-CORRECT-REPORT-ROTATION-POINTS* = 1
  ?*EXPLORATION-CORRECT-REPORT-ZONE-POINTS* = 1
  ?*EXPLORATION-WRONG-REPORT-ROTATION-POINTS* = -1
  ?*EXPLORATION-WRONG-REPORT-ZONE-POINTS* = -1
  ?*EXPLORATION-INVALID-REPORT-POINTS* = -6
  ?*PRODUCTION-WRONG-TEAM-MACHINE-POINTS* = -2
	?*PRODUCTION-POINTS-ADDITIONAL-BASE* =  2
	?*PRODUCTION-POINTS-FINISH-CC0-STEP* =  5
	?*PRODUCTION-POINTS-FINISH-CC1-STEP* = 10
	?*PRODUCTION-POINTS-FINISH-CC2-STEP* = 20
	?*PRODUCTION-POINTS-DELIVER-C0* = 20
	?*PRODUCTION-POINTS-DELIVER-C1* = 30
	?*PRODUCTION-POINTS-DELIVER-C2* = 50
	?*PRODUCTION-POINTS-DELIVER-C3* = 100
	?*PRODUCTION-POINTS-MOUNT-CAP* = 10
	?*PRODUCTION-POINTS-RETRIEVE-CAP* = 2
	?*PRODUCTION-POINTS-DELIVERY-WRONG* = 0
  ?*PRODUCTION-POINTS-DELIVER-LATE-POINTS-DEDUCTION-REL* = 0.75
  ?*PRODUCTION-POINTS-DELIVER-LATE-POINTS-STEPS* = 4 ; needs to be > 0
  ?*PRODUCTION-POINTS-COMPETITIVE-FIRST-BONUS* = 10
  ?*PRODUCTION-POINTS-COMPETITIVE-SECOND-DEDUCTION* = 10
  ?*PRODUCTION-POINTS-SS-RETRIEVAL* = -5
  ?*PRODUCTION-POINTS-SS-STORAGE* = -5
  ?*PRODUCTION-POINTS-SS-RELOCATION* = 0
  ?*PRODUCTION-POINTS-SS-PER-STORED-VOLUME* = 0
  ; Set to TRUE to cap the number of points deducted at the storage station
  ?*PRODUCTION-POINTS-SS-USE-MAX-POINT-LIMIT* = FALSE
  ?*PRODUCTION-POINTS-SS-MAX-TOTAL-POINTS* = -20
  ?*SS-PAYMENT-INTERVAL* = 60.
  ; first payment is due ?*SS-PAYMENT-INTERVAL* after ?*SS-STORAGE-GRACE-PERIOD*.
  ?*SS-STORAGE-GRACE-PERIOD* = 0.
  ?*SS-MAX-NUM-PAYMENTS-PER-VOLUME* = 5

	; Workpiece ranges
	?*WORKPIECE-RANGE-RED* = (create$ 1001 1999)
	?*WORKPIECE-RANGE-BLACK* = (create$ 2001 2999)
	?*WORKPIECE-RANGE-SILVER* = (create$ 3001 3999)
	?*WORKPIECE-RANGE-CLEAR* = (create$ 4001 4999)
	?*WORKPIECE-RANGE-UNKNOWN* = (create$ 0001 999)
  ; Setup light effects
  ?*SETUP-LIGHT-PERIOD* = 1.0
  ?*SETUP-LIGHT-PERIOD-1* = 0.5
  ?*SETUP-LIGHT-PERIOD-2* = 0.25
  ?*SETUP-LIGHT-SPEEDUP-TIME-1* = 240
  ?*SETUP-LIGHT-SPEEDUP-TIME-2* = 270
  ?*SETUP-LIGHT-MACHINES* = (create$ BS DS SS RS1 RS2 CS1 CS2)
  ; number of allowed robot maintenance cycles
  ?*MAINTENANCE-ALLOWED-CYCLES* = 2
  ?*MAINTENANCE-COST*           = (create$ 0 5)
  ?*MAINTENANCE-ALLOWED-TIME*   = 120
  ?*MAINTENANCE-WARN-TIME*      = 105
  ?*MAINTENANCE-GRACE-TIME*     =  15
  ; Game phase time; seconds
  ?*SETUP-TIME*           = 300
  ?*EXPLORATION-TIME*     = (config-get-int "/llsfrb/game/exploration-time")
  ?*PRODUCTION-TIME*      = 1200
  ?*PRODUCTION-OVERTIME*  = 300
  ?*PRODUCTION-PREPARE-TIMEOUT*  = 5
  ; Machine distribution
  ?*RANDOMIZE-GAME* = TRUE
	?*RANDOMIZE-STEPS-MACHINES* = 2
	?*RANDOMIZE-ACTIVATE-ALL-AT-START* = FALSE
	?*MACHINE-GENERATION-TIMEOUT-CHECK-STATE* = 2
	; Incremental randomization probability for switching the machines across
	; field halfs. A value from 0 to 10, 0 no change, 10, always change
	?*RANDOMIZE-INTER-SIDE-SWAP-PROB* = 3
	?*MACHINE-RANDOMIZE-TYPES* = (create$ RS CS)
  ?*MACHINE-ZONES-CYAN* = (create$ C_Z18 C_Z28 C_Z38 C_Z48 C_Z58 C_Z68 C_Z78
    C_Z17 C_Z27 C_Z37 C_Z47 C_Z57 C_Z67 C_Z77
    C_Z16 C_Z26 C_Z36 C_Z46 C_Z56 C_Z66 C_Z76
    C_Z15 C_Z25 C_Z35 C_Z45 C_Z55 C_Z65 C_Z75
    C_Z14 C_Z24 C_Z34 C_Z44 C_Z54 C_Z64 C_Z74
    C_Z13 C_Z23 C_Z33 C_Z43 C_Z53 C_Z63 C_Z73
    C_Z12 C_Z22 C_Z32 C_Z42 C_Z52 C_Z62 C_Z72
    C_Z11 C_Z21 C_Z31 C_Z41)
  ?*MACHINE-ZONES-MAGENTA* = (create$ M_Z18 M_Z28 M_Z38 M_Z48 M_Z58 M_Z68 M_Z78
    M_Z17 M_Z27 M_Z37 M_Z47 M_Z57 M_Z67 M_Z77
    M_Z16 M_Z26 M_Z36 M_Z46 M_Z56 M_Z66 M_Z76
    M_Z15 M_Z25 M_Z35 M_Z45 M_Z55 M_Z65 M_Z75
    M_Z14 M_Z24 M_Z34 M_Z44 M_Z54 M_Z64 M_Z74
    M_Z13 M_Z23 M_Z33 M_Z43 M_Z53 M_Z63 M_Z73
    M_Z12 M_Z22 M_Z32 M_Z42 M_Z52 M_Z62 M_Z72
    M_Z11 M_Z21 M_Z31 M_Z41)

  ?*FIELD-WIDTH*  = (config-get-int "/llsfrb/game/field/width")
  ?*FIELD-HEIGHT* = (config-get-int "/llsfrb/game/field/height")
  ?*FIELD-MIRRORED* = (config-get-bool "/llsfrb/game/field/mirrored")


  ?*ORDER-PRODUCTION-WINDOW-START-C0*  = 60
  ?*ORDER-PRODUCTION-WINDOW-START-C1*  = 120
  ?*ORDER-PRODUCTION-WINDOW-START-C2*  = 300
  ?*ORDER-PRODUCTION-WINDOW-START-C3*  = 400
  ?*ORDER-PRODUCTION-WINDOW-END-C0*  = 120
  ?*ORDER-PRODUCTION-WINDOW-END-C1*  = 300
  ?*ORDER-PRODUCTION-WINDOW-END-C2*  = 400
  ?*ORDER-PRODUCTION-WINDOW-END-C3*  = 500

  ?*ORDER-DELIVERY-WINDOW-START-C0*  = 90
  ?*ORDER-DELIVERY-WINDOW-START-C1*  = 90
  ?*ORDER-DELIVERY-WINDOW-START-C2*  = 150
  ?*ORDER-DELIVERY-WINDOW-START-C3*  = 150
  ?*ORDER-DELIVERY-WINDOW-END-C0*  = 180
  ?*ORDER-DELIVERY-WINDOW-END-C1*  = 180
  ?*ORDER-DELIVERY-WINDOW-END-C2*  = 210
  ?*ORDER-DELIVERY-WINDOW-END-C3*  = 210

  ?*ORDER-ACTIVATION-DISTANCE*  = 120
  ?*ORDER-ACTIVATION-DEVIATION*  = 90

  ?*ORDER-ACTIVATE-LATEST-TIME*  = 960

  ?*AGENT-TASK-ROUTER*  = debug

  ?*MOCKUP-READY-AT-OUTPUT-TIMEOUT* = 15

  ; index notation for mps generation
  ?*BASE-STATION* = 1
  ?*CAP1-STATION* = 2
  ?*CAP2-STATION* = 3
  ?*RING1-STATION* = 4
  ?*RING2-STATION* = 5
  ?*STORAGE-STATION* = 6
  ?*DELIVERY-STATION* = 7
)
