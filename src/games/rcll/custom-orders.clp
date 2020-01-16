;-- force activate custom-orders
(defrule load-custom-order
  (not (custom-order))
=>
  (assert (custom-order))
)

;--
;-- TEMPLATE DEFS
;--
(deftemplate orderer-client-response-info
  (slot client (type INTEGER))
  (slot extern-id (type INTEGER))
  (slot product-count (type INTEGER))
)

(deftemplate orderer-client-response-product-info
  (slot extern-id (type INTEGER))
  (slot product-id (type INTEGER))
  (slot quantity (type INTEGER))
  (slot last-delivered (type FLOAT))
)

;--
;-- FUNCTIONS
;--

(deffunction id-from-gensym
  ()

  ;-- build id as string
  (bind ?idstr (str-cat (gensym*)))
  (bind ?idstr-len (str-length ?idstr))

  ;-- turn to interger and add order offset
  (bind ?id (integer (string-to-field (sub-string 4 ?idstr-len ?idstr))))
  (bind ?id (+ ?id (integer 10))) ;-- add init order offset

  (return ?id)
)

(deffunction assert-custom-order
  (?order ?order-time)

  ;-- gather data from message object
  (bind ?custom-order-id (id-from-gensym))
  (bind ?gate (random 1 3))
  (bind ?complexity (pb-field-value ?order "complexity"))
  (bind ?base-color (pb-field-value ?order "base_color"))
  (bind ?cap-color (pb-field-value ?order "cap_color"))
  (bind ?quantity (pb-field-value ?order "quantity_requested"))
  (bind ?ring-colors (create$))
  (progn$ (?color (pb-field-list ?order "ring_colors"))
    (bind ?ring-colors (append$ ?ring-colors  ?color))
  )

  ;-- compose and insert order fact
  (assert (order (id ?custom-order-id) 
                 (complexity ?complexity)
                 (competitive FALSE)
                 (base-color ?base-color)
                 (ring-colors ?ring-colors)
                 (cap-color ?cap-color)
                 (quantity-requested ?quantity) 
                 (quantity-delivered 0 0)
                 (start-range 0 0) 
                 (duration-range 0 0)
                 (delivery-period (+ ?order-time 5.0) 1020)
                 (delivery-gate ?gate)
                 (active FALSE) ;-- TODO: determine when this gets activated 
                 (activate-at (+ ?order-time 5.0))
                 (activation-range 0 0)
                 (allow-overtime FALSE)
         )
  )

  (return ?custom-order-id)
)

(deffunction copy-order
  (?orig)
  
  (bind ?copy (pb-create "llsf_msgs.Order"))
  
  (pb-set-field ?copy "id" (pb-field-value ?orig "id"))
  (pb-set-field ?copy "complexity" (pb-field-value ?orig "complexity"))
  (pb-set-field ?copy "base_color" (pb-field-value ?orig "base_color"))
  (pb-set-field ?copy "cap_color" (pb-field-value ?orig "cap_color"))
  (pb-set-field ?copy "quantity_requested" (pb-field-value ?orig "quantity_requested"))
  (pb-set-field ?copy "quantity_delivered_cyan" (pb-field-value ?orig "quantity_delivered_cyan"))
  (pb-set-field ?copy "quantity_delivered_magenta" (pb-field-value ?orig "quantity_delivered_magenta"))
  (pb-set-field ?copy "delivery_period_begin" (pb-field-value ?orig "delivery_period_begin"))
  (pb-set-field ?copy "delivery_period_end" (pb-field-value ?orig "delivery_period_end"))
  (pb-set-field ?copy "delivery_gate" (pb-field-value ?orig "delivery_gate"))
  (pb-set-field ?copy "competitive" FALSE)
  
  (return ?copy)
)

(deffunction get-orderinfo-extid
  (?orderinfo)
  
  (bind ?eid 0)
  
  ;(bind ?orders (pb-field-list ?orderinfo "orders"))
  ;(bind ?o (first$ ?orders)
  ;(bind ?eid (pb-field-value ?o "id"))

  (foreach ?o (pb-field-list ?orderinfo "orders")
    (bind ?eid (pb-field-value ?o "id"))
    (break)
  )
    
  (return ?eid)
)

;--
;-- RULES
;--

(defrule print-custom-order-success
  (custom-order)
=>
  (printout warn "INFO: Enabling custom-order was successful" crlf)
)

(defrule remove-initial-orders
  (custom-order)
  ?of <- (order (id ?id&:(< ?id 10))
                (start-range ?ss&:(< ?ss 1320) ?se&:(<= ?se 1320))
         ) 
=>
  (printout warn "INFO: Remove initial order id " ?id crlf)

  ;-- remove original game order
  (retract ?of)  
)

(defrule net-recv-custom-OrderInfo
  (custom-order)
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?now))
  ?pf <- (protobuf-msg (type "llsf_msgs.OrderInfo")
            (ptr ?ptr)
            (rcvd-via STREAM)
            (rcvd-from ?from-host ?from-port)
            (client-id ?cid)
         )
  ;-- reject already processing orders
  (not (orderer-client-response-info 
          (extern-id ?eid&:(eq ?eid (get-orderinfo-extid ?ptr)))
        )
  )
  
=>

  (printout warn "INFO: Received order information from client " ?cid crlf)
  
  ;-- insert orders from message
  (bind ?recv-resp (pb-create "llsf_msgs.Order"))
  (bind ?eid -1)
  (bind ?cnt 0)
  (foreach ?o (pb-field-list ?ptr "orders")
    (if (< ?eid 0) then
      (bind ?recv-resp (copy-order ?o))
      (bind ?eid (pb-field-value ?o "id"))
    )

    (bind ?id 
      (assert-custom-order ?o ?now)
    )
    
    ;-- increase product count
    (bind ?cnt (+ ?cnt 1))
    
    ;-- assert orderer-client-response-product-info
    (bind ?quantity (pb-field-value ?o "quantity_requested"))
    (assert (orderer-client-response-product-info
              (extern-id ?eid)
              (product-id ?id)
              (quantity ?quantity)
              (last-delivered 0.0)
            )
    )
    
    (printout warn "INFO: Append " ?quantity "x of product " ?id " to external order " ?eid crlf)
  )
  
  ;-- remember orders and orderer (client)
  (assert (orderer-client-response-info
            (client ?cid) 
            (extern-id ?eid) 
            (product-count ?cnt)
          )
  )
    
  (printout warn "INFO: Inserted external order " ?eid " with " ?cnt " products" crlf)
  (pb-send ?cid ?recv-resp)
  (pb-destroy ?recv-resp)
)

(defrule custom-product-delivered
  ?pf <- (product-processed (game-time ?gt) (team ?team) (order ?order) (confirmed TRUE)
                            (workpiece ?wp-id) (mtype DS) (scored FALSE))
  ?of <- (order (id ?order))
  ?roi <- (orderer-client-response-product-info 
            (extern-id ?eid) 
            (product-id ?id)
            ;-- prevent triggering when quantity is already reached
            (quantity    ?quantity&:(> ?quantity 0))
            ;-- prevent triggering same product-delivered multiple times
            (last-delivered ?time&~?gt)
          )
  ?ri <- (orderer-client-response-info 
           (extern-id ?eid) 
           (product-count ?ocnt)
         )
=>
  ;-- decrease remaining quanties
  (bind ?updquant (- ?quantity 1))
  (modify ?roi 
            (quantity ?updquant)
            (last-delivered ?gt)
  )
  (printout warn "Delivered 1 product of " ?id " (" ?updquant " remaining)" crlf)
)

(defrule custom-all-of-product-delivered
  ?pf <- (product-processed (order ?order&~0))
  ?of <- (order (id ?order))
  ?roi <- (orderer-client-response-product-info 
            (extern-id ?eid) 
            (product-id ?id)
            (quantity 0)
          )
  ?ri <- (orderer-client-response-info 
           (extern-id ?eid) 
           (product-count ?ocnt)
         )
=>
  ;-- decrease amount of remaining products
  (bind ?updcnt (- ?ocnt 1))

  ;-- remove response order info
  (retract ?roi)
  (modify ?ri (product-count ?updcnt))
  (printout warn "Delivered all products of " ?id " from " ?eid " (" ?updcnt " prod. remaining)" crlf)
)

(defrule custom-order-complete
  (custom-order)
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?ri <- (orderer-client-response-info 
           (client ?cid) 
           (extern-id ?eid) 
           (product-count 0)
         )
=>
  ;-- forget order and orderer
  (retract ?ri)

  (printout warn "INFO: Responded to completed order " ?eid ", ordered by " ?cid crlf)

  ;-- compose message to inform controller about delivery
  (bind ?delivery-msg (pb-create "llsf_msgs.SetOrderDelivered"))
  (pb-set-field ?delivery-msg "order_id" ?eid)
  (pb-set-field ?delivery-msg "team_color" CYAN) ;-- this is ignored

  ;-- actually send message
  ;(pb-broadcast ?cid ?delivery-msg)
  (pb-send ?cid ?delivery-msg)
  (pb-destroy ?delivery-msg)
)
