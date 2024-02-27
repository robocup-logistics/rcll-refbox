
;---------------------------------------------------------------------------
;  priorities.clp - refbox rule priorities
;
;  Created: Mon Feb 11 12:36:48 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defglobal
  ?*PRIORITY-FIRST*   =  5000
  ?*PRIORITY-HIGHER*  =  1000
  ?*PRIORITY-HIGH*    =   500
  ?*PRIORITY-CLEANUP* = -4000
  ?*PRIORITY-LAST*    = -5000
)
