# Using the config files
Ensure that when you want to run a challenge line 152 is set to true:
````yaml
mongodb:
  enable: true
````
and line 183:
````yaml
challenges:
  enable: true
````
and line 266:
````yaml
webshop:
  enable: true
````
## Add your own Teamname
The preconfigured Teamname is _Carologistics_, you need to chang it to your teamname.
In all _config_XXX.yaml_ files change line 160:  
````yaml
game:
  teams: [Carologistics]
````
to 
````yaml
game:
  teams: [The name of your team]
````

## Challenge name

In order to process the gamereports later you need to fill out the name field in line 184:
````yaml
name: "TEAMNAME_CHALLENGE_DIFFICULTY"
````
## Load a custom config file

The RefBox only loads files called _config.yaml_.
If you want your file to be loaded:
````bash
cp config_XXX.yaml config.yaml
````

## Editing files
The challenges are configured in the challenges field (line 182):
````yaml
challenges:
    enable: false # change to true to use RefBox challenge mode
    name: ""
    # max is 7 x 8
    field:
      width: 5
      height: 5
      # if true this effectively doubles the field size
      mirror: false
    # Phases in which the machine positions are broadcasted
    send-mps-ground-truth: []

    production-time: 1020
    orders:
      # if disabled, use default orders from rcll
      customize: true
      # each order is named O<INTEGER>, just add your own orders underneath
      O1:
        base-color: BASE_RED
        ring-colors: [RING_BLUE, RING_YELLOW, RING_ORANGE]
        cap-color: CAP_BLACK
    publish-routes: # publish route for the navigation challenge
      enable: true # true if you want to do the navigation challenge, false if not
      num-points: 12
      num-routes: 1
      pause-duration: 5

      # orders are randomized according to the given spec
    machines: [BS, CS1, RS1, RS2] # number of machines determines the challenge difficulty (2=easy, 4=hard)
    machine-setup:
      customize: true # true if you want to use the setup from below, false=random
      rs1-colors: [RING_BLUE, RING_YELLOW]
      rs2-colors: [RING_ORANGE, RING_GREEN]
      ring-specs: # payments for the rings
        RING_BLUE: 1
        RING_YELLOW: 0
        RING_ORANGE: 2
        RING_GREEN: 0
````