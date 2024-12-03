# The MQTT Communication Standard
This MQTT setup is intended to be used with the MPS using Codesys. Compared to "mqtt_legacy," this standard is human-readable.

### New MQTT Setup (for legacy support use type "mqtt_legacy"):
```
MPS/{NAME}/Command //The refbox only sends here it's commands
MPS/{NAME}/Status //The refbox can read the status here
MPS/{NAME}/WP-Sensor //The refbox reads whether or not a wp is on the output of a mps (for bs also input or output)
MPS/{NAME}/Barcode //The refbox can read the last scanned Barcode here
MPS/{NAME}/SlideCount //The refbox can read the SlideCount on Ringstations here
MPS/{NAME}/Sensors/... //(OPTIONAL) here can sensors be published for debugging and logging purposes not to be used by refbox directly
```

#### The only valid names are:
```
NAME = {C-CS1, C-CS2, C-RS1, C-RS2, C-DS, C-BS, C-SS, M-CS1, M-CS2, M-RS1, M-RS2, M-DS, M-BS, M-SS}
```

#### The only valid Barcode
6 digit integer

#### The only valid SlideCount
A integer that counts up every time the sensor is activated

##### The only valid Status are:
```
Status = {IDLE, BUSY} // IDLE mean machine has nothing to do, BUSY means that the machine is physically busy with processing an intruction
```

#### The only avalid WP-Sensor are:

```
WP-Sensor = {NoWP, WP} //For bs input or output for any other machine just if there is a wp at the output
```

#### A valid command looks like this:
```
command = "{ACTION} [{ARG1} {ARG2}]"
```

#### The only valid Actions:
```
ACTION = {"GET_BASE", "MOUNT_RING", "CAP_ACTION", "DELIVER", "RETRIEVE", "LIGHT", "STORE", "RELOCATE", "MOVE_CONVEYOR", "RESET"}
```

#### Args for each command:
```
GET_BASE {RED|BLACK|SILVER}
LIGHT {RED|GREEN|YELLOW|RESET} {ON|OFF|BLINK} //RESET DOES NOT HAVE A 2nd ARG
CAP_ACTION {RETRIEVE|MOUNT}
MOUNT_RING {RING1|RING2}
DELIVER {SLOT1|SLOT2|SLOT3}
STORE {TARGET}
RETRIEVE {TARGET} //UNSTORE FROM A STORAGE STATION
RELOCATE {FROM} {TARGET}
MOVE_CONVEYOR {TO_INPUT|TO_OUTPUT} {IN|MID|OUT} // first direction and then target MPS should execute that without thinking(i.e. TO_INPUT OUTPUT would be a valid command)
RESET
```

##### A storage station Target is:
```
SHELF,SLOT where SHELF = {0,1,2,3,4,5} AND SLOT = {0,1,2,3,4,5,6,7} 
```

### Intendet Parsing

``` sudocode
args = mqtt.command.split(" ")
switch(args[0]):
    case("LIGHT"):
        if(arg[1] == "RED"){
            if(arg[2] == "ON")
                light.red = LIGHT::ON;
                .....
        break;
    case("MOUNT_RING"):
        ....
        break;
    case(...)
    default:
        Logger.Error("Unknown Command going to be ignored")
        break;
        ....
```

### TODO Discovery
MQTT needs a way to tell MPS's who forgot their role to tell them which role they are.

This is currently not implemented.
