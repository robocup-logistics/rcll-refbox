## Compiler stuff
CC	 	 =	gcc
CPPFLAGS 	 =	-g3
LDFLAGS	 	 =	-lmodbus -lstdc++
SILENT   	 = 	@
ECHO 	 	 =	echo
OBJECTCOM	 = 	mps.o mps_info.o mps_list.o mps_refbox_interface.o mps_pick_place_1.o mps_pick_place_2.o mps_deliver.o mps_incoming_station.o mps_band.o 
MODULES		 = 	 modbuscom
INCLUDE		 = 	-I./include/

all: $(MODULES)

modbuscom: $(OBJECTCOM)
	 $(SILENT) $(ECHO) "--- Creating $@ ---"
	 $(SILENT) $(CC) $^ -o $@  $(LDFLAGS)

%.o: %.cpp
	$(SILENT) echo "---Compiling $< ---"
	$(SILENT) $(CC) -c $< $(CPPFLAGS) $(INCLUDE) 

clean: 
	$(SILENT)rm -f $(MODULES)
	$(SILENT)rm -f *.o
