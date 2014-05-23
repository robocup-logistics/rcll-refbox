/***************************************************************************
 *  btr-view2.h - output the communication data from ProtoBuf.
 *
 *  Created: Mon June 17 14:20:00 2013
 *  Copyright  2013  Wataru UEMURA [friede.elec.ryukoku.ac.jp]
 ****************************************************************************/

//
// refbox-view2 creates the file which includes the received data.
// 
#ifndef BTR_VIEW2
#define BTR_VIEW2

typedef struct
{
  std::string filename;		// output filename 
  int phase;
  int state;
  int quantity_requested[3];	// order infor at Produciton Phase
  int quantity_delivered[3];	// order info at Production Phase
  int lspec_state[5][3];	// LightColor information at Exploration phase	
  int mType[10];		// Machine Type at Production Phase
} btrview2send;

typedef struct
{
  std::string filename;	// input filename
  int machineNumber;	// report machine number
  int machineType;	// report machine Type
  double x;		// location of robotino
  double y;
  double ori;
  unsigned long seq;
} btrview2recv;


#endif
