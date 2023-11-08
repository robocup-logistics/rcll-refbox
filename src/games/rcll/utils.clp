
;---------------------------------------------------------------------------
;  utils.clp - CLIPS agent utility functions
;
;  Created: Sun Jun 17 12:19:34 2012 (Mexico City)
;  Copyright  2012  Tim Niemueller [www.niemueller.de]
;  Licensed under GPLv2+ license, cf. LICENSE file
;---------------------------------------------------------------------------
(defglobal
	?*LINE-WIDTH* = 80
)

(deffunction debug (?level)
  (return (<= ?level ?*DEBUG*))
)

(deffunction append$ (?list $?items)
  (insert$ ?list (+ (length$ ?list) 1) ?items)
)

(deffunction str-replace (?input ?pattern ?replace)
" Replace all occurrences of a pattern in a string
  @param ?input: string where pattern is replaced
  @param ?pattern: pattern to replace
  @param ?replace: pattern to replace
"
	(bind ?start 1)
	(bind ?pos (str-index ?pattern ?input))
	(while (neq ?pos FALSE)
		(bind ?pos (+ ?pos ?start -1))
		(bind ?input
		  (str-cat (sub-string 1 (- ?pos 1) ?input)
		    ?replace
		    (sub-string (+ ?pos (str-length ?pattern)) (length$ ?input) ?input)))
		(bind ?start (+ ?pos (str-length ?pattern)))
		(bind ?pos (str-index ?pattern (sub-string ?start (length$ ?input) ?input)))
	)
	(return ?input)
)

(deffunction snake-case (?input)
" Converts input strint to snake case"
  (return (str-replace (lowcase ?input) "-" "_"))
)

(deffunction randomize-tuple-list$ (?list ?tuple-length)
" Randomize a list of n-tuples such that each tuple stays connected
  @param ?list: List of n-tuples (hence its length is divisible by n)
  @param ?tuple-length: Length n of each tuple
  @return: random order of the n-tuples, e.g., (1 2 3 4) 2 -> (3 4 1 2) or (1 2 3 4)
"
	(bind ?l (/ (length$ ?list) ?tuple-length))
	(loop-for-count 200 do
		(bind ?a (random 1 ?l))
		(bind ?b (random 1 ?l))
		(loop-for-count (?partial 0 (- ?tuple-length 1)) do
			(bind ?a-curr (- (* ?a ?tuple-length) ?partial))
			(bind ?b-curr (- (* ?b ?tuple-length) ?partial))
			(bind ?tmp (nth$ ?a-curr ?list))
			(bind ?list (replace$ ?list ?a-curr ?a-curr (nth$ ?b-curr ?list)))
			(bind ?list (replace$ ?list ?b-curr ?b-curr ?tmp))
		)
	)
	(return ?list)
)

(deffunction randomize$ (?list)
  (bind ?l (length$ ?list))
  (loop-for-count 200 do
    (bind ?a (random 1 ?l))
    (bind ?b (random 1 ?l))
    (bind ?tmp (nth$ ?a ?list))
    (bind ?list (replace$ ?list ?a ?a (nth$ ?b ?list)))
    (bind ?list (replace$ ?list ?b ?b ?tmp))
  )
  (return ?list)
)

(deffunction mirror-name (?zn)
  (bind ?team (sub-string 1 1 ?zn))
  (bind ?machine (sub-string 3 99 ?zn))
  (if (eq ?team "M") then
    (return (sym-cat "C-" ?machine))
  else
    (return (sym-cat "M-" ?machine))
  )
)

(deffunction mirror-zone (?zn)
  (bind ?team (sub-string 1 1 ?zn))
  (bind ?zone (sub-string 3 99 ?zn))
  (if (eq ?team "M") then
    (return (sym-cat "C_" ?zone))
  else
    (return (sym-cat "M_" ?zone))
  )
)

(deffunction pose-to-zone (?x ?y)
	(if (< ?x 0)
	 then
		(bind ?x-zone (+ (* -1 (integer ?x)) 1))
		(bind ?side "M_Z")
	 else
		(bind ?x-zone (+ (integer ?x) 1))
		(bind ?side "C_Z")
	)
	(bind ?y-zone (+ (integer ?y) 1))
	(return (sym-cat ?side ?x-zone ?y-zone))
)

(deffunction want-mirrored-rotation (?mtype ?zone)
"According to the RCLL2017 rulebook, this is when a machine is mirrored"
  (bind ?zn (str-cat ?zone))
  (bind ?x (eval (sub-string 4 4 ?zn)))
  (bind ?y (eval (sub-string 5 5 ?zn)))

  (return (or (member$ ?mtype (create$ BS DS SS))
              (not (or (eq ?x ?*FIELD-WIDTH*) ; left or right
                       (eq ?y ?*FIELD-HEIGHT*) ; top wall
                       (eq ?y 1) ; bottom wall
                       (and (<= ?x ?*FIELD-WIDTH*) ; insertion
                            (>= ?x (- ?*FIELD-WIDTH* 2))
                            (eq ?y 2)
                       )
                   )
              )
  ))
)

(deffunction mirror-orientation (?mtype ?zone ?ori)
  (bind ?zn (str-cat ?zone))
  (bind ?t (sub-string 1 1 ?zn))
  (if (want-mirrored-rotation ?mtype ?zone)
   then
    (if (eq ?t "C")
     then
      (do-for-fact ((?mo mirror-orientation)) (eq ?mo:cyan ?ori)
        (bind ?m-ori ?mo:magenta)
      )
     else
      (do-for-fact ((?mo mirror-orientation)) (eq ?mo:magenta ?ori)
        (bind ?m-ori ?mo:cyan)
      )
    )
    (return ?m-ori)
   else
    (bind ?x (eval (sub-string 4 4 ?zn)))
    (bind ?y (eval (sub-string 5 5 ?zn)))

    (if (eq ?y ?*FIELD-HEIGHT*) then
      (return 180)
    )
    (if (or (eq ?y 1) (eq ?y 2)) then
      (return 0)
    )
    (if (and (eq ?x ?*FIELD-WIDTH*) (eq ?t "M")) then  ; this is the other way around, because I compare with the team color of the originalting machine
      (return 90)
    )
    (if (and (eq ?x ?*FIELD-WIDTH*) (eq ?t "C")) then
      (return 270)
    )
    (printout error "error in rotation of machines, checked all possible cases, but nothing cateched" crlf)
    (return ?ori)
  )
)

(deffunction pick-random$ (?list)
  (return (nth$ (random 1 (length$ ?list)) ?list))
)

(deffunction remove$ (?list ?item)
	(bind ?idx (member$ ?item ?list))
	(if ?idx
			then	(return (delete$ ?list ?idx ?idx))
			else  (return ?list))
)

(deffunction is-even-int (?num)
  (return (eq (mod ?num 2) 0))
)

(deffunction is-odd-int (?num)
  (return (eq (mod ?num 2) 1))
)

(deffunction non-zero-pose ($?pose)
  (foreach ?pf ?pose (if (<> ?pf 0.0) then (return TRUE)))
  (return FALSE)
)


(deffunction in-box (?pose ?box-center ?box-extent)
  (bind ?box-half-x (/ (nth$ 1 ?box-extent) 2))
  (bind ?box-half-y (/ (nth$ 2 ?box-extent) 2))

  (return (and (<= (abs (- (nth$ 1 ?pose) (nth$ 1 ?box-center))) ?box-half-x)
	       (<= (abs (- (nth$ 2 ?pose) (nth$ 2 ?box-center))) ?box-half-y)))
)

(deffunction string-gt (?s1 ?s2)
  (return (> (str-compare ?s1 ?s2) 0))
)


(deffunction gen-random-string (?l)
  (bind ?rv "")
  (bind ?random-set (create$ 33 35 36 45 46 58 61 64 95))
  (loop-for-count (?c 48  57) (bind ?random-set (append$ ?random-set ?c)))
  (loop-for-count (?c 65  90) (bind ?random-set (append$ ?random-set ?c)))
  (loop-for-count (?c 97 122) (bind ?random-set (append$ ?random-set ?c)))

  (loop-for-count (?c 1 ?l)
    (bind ?rv (str-cat ?rv (format nil "%c" (pick-random$ ?random-set))))
  )
  (return ?rv)
)

(deffunction ceil (?f)
  (bind ?rf (round ?f))
  (return (if (>= (- ?rf ?f) 0) then ?rf else (+ ?rf 1)))
)

(deffunction floor (?f)
  (bind ?rf (round ?f))
  (return (if (> (- ?rf ?f) 0) then (- ?rf 1) else ?rf))
)

(deffunction type-cast (?value ?type)
" Convert a value to a given type by a direct type cast.
  @param ?value Value to cast
  @param ?type Target type to cast
  @return Value of ?value with ?type
"
	(switch ?type
		(case INTEGER then (return (integer ?value)))
		(case STRING then (return (str-cat ?value)))
		(case FLOAT then (return (float ?value)))
		(case SYMBOL then (return (sym-cat ?value)))
	)
	(printout debug "cast: unsupported type cast: " ?type " Expected INTEGER|STRING|FLOAT|SYMBOL" crlf)
	(return ?value)
)

(deffunction type-cast-list (?list ?type)
" Convert each member of a list to a given type.
  @param ?list List to cast
  @param ?type Target type to cast
  @return List with all elements of ?list but with type ?type
"
	(bind ?res (create$))
	(foreach ?c ?list
		(bind ?res (append$ ?res (type-cast ?c ?type)))
	)
	(return ?res)
)

(deffunction assert-points-with-threshold (?upper-bounded ?prefix ?threshold ?msg ?points ?gt ?team ?phase)
" Assert points without crossing the threshold
  @param ?upper-bounded: Set to FALSE iff the threshold is a lower bound
  @param ?prefix: Prefix of the reason which determines whether a point score
                  contributes to the threshold
  @param ?threshold: Threshold that is not crossed
  @param ?msg: reason for points
  @param ?points: number of points to add
  @param ?gt: Current game time
  @param ?team: Team scoring the points
  @param ?phase: Game phase
"
	(bind ?resulting-points ?points)
	(bind ?total-points 0)
	(bind ?print-max-points-reached FALSE)
	(do-for-all-facts ((?p points))
	                  (and (eq (sub-string 1 (length ?prefix) ?p:reason) ?prefix)
	                       (eq ?p:team ?team))
	                  (bind ?total-points (+ ?total-points ?p:points))
	)
	(bind ?threshold-comp (- ?threshold (+ ?total-points ?points)))
	(if (and ?upper-bounded (> ?points 0) (<= ?threshold-comp 0))
	 then
		(bind ?resulting-points (max 0 (+ ?points ?threshold-comp)))
		(bind ?print-max-points-reached TRUE)
	)
	(if (and (not ?upper-bounded) (< ?points 0) (>= ?threshold-comp 0))
	 then
		(bind ?resulting-points (min 0 (+ ?points ?threshold-comp)))
		(bind ?print-max-points-reached TRUE)
	)
	(if (neq ?resulting-points 0)
	 then
		(assert (points (game-time ?gt) (team ?team) (phase ?phase)
		                (points ?resulting-points) (reason (str-cat ?prefix ": " ?msg))))
		(if ?print-max-points-reached
		 then
			(bind ?t-str (create$ "lower bound" "deducted"))
			(if ?upper-bounded then (bind ?t-str (create$ "upper bound" "added")))
			(printout t "Team " ?team " reached " (nth$ 1 ?t-str)" at " ?prefix ": "
			            ?threshold ". No more points are " (nth$ 2 ?t-str) "." crlf)
		)
	)
)

(deffunction ss-assert-points-with-threshold (?msg ?points ?gt ?team)
" Assert point deductions for using the SS without crossing the max threshold
  @param ?msg reason for points
  @param ?points number of points to add (negative for deductions)
  @param ?gt Current game time
  @param ?team Team scoring the points
"
	(if ?*PRODUCTION-POINTS-SS-USE-MAX-POINT-LIMIT*
	 then
		(assert-points-with-threshold FALSE
		                              "SS"
		                              ?*PRODUCTION-POINTS-SS-MAX-TOTAL-POINTS*
		                              ?msg
		                              ?points
		                              ?gt
		                              ?team
		                              PRODUCTION)
	 else
		(assert (points (game-time ?gt) (team ?team) (phase PRODUCTION)
		                (points ?points) (reason (str-cat "SS: " ?msg))))
	)
)

(deffunction ss-slot-blocked-by (?back-slot ?front-slot)
" Checks if a SS shelf slot is located in the back of another slot.
  @param ?back-slot some slot number
  @param ?front-slot some slot number
  @return TRUE iff ?back-slot is located in the back of ?front-slot
"
	(return (and (eq ?front-slot (- ?back-slot 1)) (eq 0 (mod ?front-slot 2))))
)

(deffunction ss-update-accessible-slots (?machine ?shelf ?slot ?is-free)
" Update shelf-slot accessibility status
  @param ?machine: SS which has stored/retrieved/re-positioned a product
  @param ?shelf: shelf number where the change happened
  @param ?slot: slot where the storage status changed
  @param FALSE iff the shelf-slot is now occupied
"
	(do-for-fact ((?s machine-ss-shelf-slot))
	             (and (eq ?s:name ?machine) (eq (nth$ 1 ?s:position) ?shelf)
	                  (ss-slot-blocked-by (nth$ 2 ?s:position) ?slot))
		(modify ?s (is-accessible ?is-free))
	)
)

(deffunction ss-positions-compare (?f-1 ?f-2)
" Comparison for function sort to order machine-ss-shelf-slot facts in
  ascending order based on their position
  @param ?f-1: fact adress or index of machine-ss-shelf-slot fact
  @param ?f-2: fact adress or index of machine-ss-shelf-slot fact
  @return: ?f-1 > ?f-2 where ">" compares shelf and slot positions
"
	(bind ?pos1 (fact-slot-value ?f-1 position))
	(bind ?pos2 (fact-slot-value ?f-2 position))
	(return (or (> (nth$ 1 ?pos1) (nth$ 1 ?pos2))
	            (and (= (nth$ 1 ?pos1) (nth$ 1 ?pos2))
	                 (> (nth$ 2 ?pos1) (nth$ 2 ?pos2)))))
)

; Randomize the positions of each product for each shelf
(deffunction ss-shuffle-shelves ()
	(loop-for-count (?shelf 0 ?*SS-MAX-SHELF*)
		(bind ?positions (create$))
		(do-for-all-facts ((?ssf machine-ss-shelf-slot))
			                (and (eq ?ssf:name C-SS) (eq (nth$ 1 ?ssf:position) ?shelf))
			                (bind ?positions (append$ ?positions ?ssf:position))
		)
		(bind ?positions (randomize-tuple-list$ ?positions 2))
		(do-for-all-facts ((?ss machine)) (eq ?ss:mtype SS)
			(bind ?i 1)
			(delayed-do-for-all-facts ((?ssf machine-ss-shelf-slot))
				                        (and (eq ?ss:name ?ssf:name)
				                             (eq (nth$ 1 ?ssf:position) ?shelf))
				(modify ?ssf (position (subseq$ ?positions ?i (+ ?i 1))) (move-to (subseq$ ?positions ?i (+ ?i 1)) ) )
				(bind ?i (+ ?i 2))
			)
		)
	)
	(delayed-do-for-all-facts ((?ssf machine-ss-shelf-slot)) TRUE
		(modify ?ssf (move-to (create$ ) ) )
	)
	(do-for-all-facts ((?ss machine)) (eq ?ss:mtype SS)
		(delayed-do-for-all-facts ((?ssf machine-ss-shelf-slot)) TRUE
			(ss-update-accessible-slots ?ss:name (nth$ 1 ?ssf:position) (nth$ 2 ?ssf:position) (not ?ssf:is-filled))
		)
	)
)

(deffunction ss-print-storage (?mps-ss)
" Print all stored products of a storage station
  @param ?mps-ss: name of storage station to print information about
"
	(printout t ?mps-ss " contains:" crlf)
	(bind ?filled-positions (find-all-facts ((?ssf machine-ss-shelf-slot))
		                (and ?ssf:is-filled (eq ?ssf:name ?mps-ss))))
	(bind ?filled-positions (sort ss-positions-compare ?filled-positions))
	(foreach ?f ?filled-positions
		(printout t " - " (fact-slot-value ?f position) " stores " (fact-slot-value ?f description) crlf)
	)
)

(deffunction print-sep (?header)
" Prints a dashed line with centered header of constant width "
	(bind ?length (str-length ?header))
	(bind ?pre-padding-length (/ (- ?*LINE-WIDTH* ?length) 2))
	(bind ?post-padding-length (- ?*LINE-WIDTH* (+ ?length ?pre-padding-length)))
	(bind ?pre-padding "")
	(loop-for-count ?pre-padding-length
		(bind ?pre-padding (str-cat ?pre-padding "-"))
	)
	(bind ?post-padding "")
	(loop-for-count ?post-padding-length
		(bind ?post-padding (str-cat ?post-padding "-"))
	)
	(printout t crlf ?pre-padding ?header ?post-padding crlf crlf)
)

(deffunction fact-indices (?fact-addresses)
" Returns fact indices to a given list of fact addresses."
	(bind ?indices (create$))
	(progn$ (?m-f ?fact-addresses) (bind ?indices (append$ ?indices (fact-index ?m-f))))
	(return ?indices)
)

(deffunction get-machine-meta-fact (?machine-f)
" For a given machine fact return the corresponding meta fact or nil if it does
  not exist.
"
  (bind ?meta-f (eval
    (str-cat "(find-fact ((?meta " (sym-cat (lowcase (fact-slot-value ?machine-f mtype)) -meta) "))"
    "(eq " (fact-slot-value ?machine-f name) " ?meta:name))")
  ))
  (if (= (length$ ?meta-f) 1)
   then (return (nth$ 1 ?meta-f))
   else
    (printout error "get-machine-meta-fact failed to find the meta fact for " ?machine-f crlf)
    (return nil)
  )
)

(deffunction pack-value-to-string (?value ?type)
" Convert a value or list of values to a string, such that CLIPS can retrieve
  the type later on.
  Useful helper when using 'assert-string'.
  @param ?value Value or list of values
  @param ?type Type of the value(s) of ?value
  @return string packed with the value(s)
"
	(bind ?raw-val ?value)
	(bind ?is-list (eq (type ?value) MULTIFIELD))
	(if ?is-list
	 then
		(bind ?tmp ?raw-val)
		(progn$ (?f ?tmp) (bind ?raw-val (replace$ ?raw-val ?f-index ?f-index (type-cast ?f ?type))))
		(bind ?typed-string (implode$ ?raw-val))
	 else
		(bind ?typed-string (str-cat (type-cast ?value ?type)))
		(if (eq ?type STRING) then (bind ?typed-string (str-cat "\"" ?typed-string "\"")))
	)
	(return ?typed-string)
)


(deffunction fact-to-string (?fact)
	(bind ?template (fact-relation ?fact))
	(bind ?update-str (str-cat "(" ?template))
	(foreach ?slot (deftemplate-slot-names ?template)
		(bind ?is-multislot (deftemplate-slot-multip ?template ?slot))
		(bind ?types (deftemplate-slot-types ?template ?slot))
		(if (neq (length$ ?types) 1)
		 then
			(printout error "fact-to-string: type of slot " ?slot
			                " of template "  ?template " cannot be determined, skipping." crlf)
		 else
			(bind ?type (nth$ 1 ?types))
			(bind ?value (fact-slot-value ?fact ?slot))
			(bind ?value (pack-value-to-string ?value ?type))
			(bind ?update-str (str-cat ?update-str " (" ?slot " " ?value ")"))
		)
	)
	(bind ?update-str (str-cat ?update-str ")"))
  (return ?update-str)
)
