
;---------------------------------------------------------------------------
;  priorities.clp - refbox rule priorities
;
;  Created: Mon Feb 11 12:36:48 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defglobal
  ?*PRIORITY_FIRST*   =  5000
  ?*PRIORITY_HIGHER*  =  1000
  ?*PRIORITY_HIGH*    =   500
  ?*PRIORITY_CLEANUP* = -4000
  ?*PRIORITY_LAST*    = -5000
)
