
/***************************************************************************
 *  machines.cpp - LLSF machine names to numbers
 *
 *  Created: Mon Mar 24 12:21:11 2014
 *  Copyright  2013-2014  Tim Niemueller [www.niemueller.de]
 ****************************************************************************/

/*  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the authors nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <utils/llsf/machines.h>

namespace llsf_utils {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

unsigned int
to_machine(std::string &machine_name,
	   MachineAssignment machine_assignment, TeamAssignment team_assignment)
{
  switch (machine_assignment) {
  case ASSIGNMENT_2013:
    if (machine_name == "M1")       return  0;
    else if (machine_name == "M2")  return  1;
    else if (machine_name == "M3")  return  2;
    else if (machine_name == "M4")  return  3;
    else if (machine_name == "M5")  return  4;
    else if (machine_name == "M6")  return  5;
    else if (machine_name == "M7")  return  6;
    else if (machine_name == "M8")  return  7;
    else if (machine_name == "M9")  return  8;
    else if (machine_name == "M10") return  9;
    else if (machine_name == "D1")  return 10;
    else if (machine_name == "D2")  return 11;
    else if (machine_name == "D3")  return 12;
    else if (machine_name == "TST") return 13;
    else if (machine_name == "R1")  return 14;
    else if (machine_name == "R2")  return 15;
    else throw fawkes::Exception("Unknown machine type %s", machine_name.c_str());
    break;

  case ASSIGNMENT_2014:
    {
      bool cyan = (team_assignment == TEAM_CYAN);

      if (machine_name == "M1")       return cyan ?  0 : 16;
      else if (machine_name == "M2")  return cyan ?  1 : 17;
      else if (machine_name == "M3")  return cyan ?  2 : 18;
      else if (machine_name == "M4")  return cyan ?  3 : 19;
      else if (machine_name == "M5")  return cyan ?  4 : 20;
      else if (machine_name == "M6")  return cyan ?  5 : 21;
      else if (machine_name == "M7")  return cyan ?  6 : 22;
      else if (machine_name == "M8")  return cyan ?  7 : 23;
      else if (machine_name == "M9")  return cyan ?  8 : 24;
      else if (machine_name == "M10") return cyan ?  9 : 25;
      else if (machine_name == "M11") return cyan ? 10 : 26;
      else if (machine_name == "M12") return cyan ? 11 : 27;
      else if (machine_name == "D1")  return cyan ? 12 : 28;
      else if (machine_name == "D2")  return cyan ? 13 : 29;
      else if (machine_name == "D3")  return cyan ? 14 : 30;
      else if (machine_name == "R1")  return cyan ? 15 : 31;
      else throw fawkes::Exception("Unknown machine type %s", machine_name.c_str());
    }
    break;

  default:
    throw fawkes::Exception("Unknown assignment/machine name");
  }
}


const char *
to_string(unsigned int machine,
	  MachineAssignment machine_assignment, TeamAssignment team_assignment)
{
  switch (machine_assignment) {
  case ASSIGNMENT_2013:
    switch (machine) {
    case  0: return "M1";
    case  1: return "M2";
    case  2: return "M3";
    case  3: return "M4";
    case  4: return "M5";
    case  5: return "M6";
    case  6: return "M7";
    case  7: return "M8";
    case  8: return "M9";
    case  9: return "M10";
    case 10: return "D1";
    case 11: return "D2";
    case 12: return "D3";
    case 13: return "TST";
    case 14: return "R1";
    case 15: return "R2";
    default:
      throw fawkes::Exception("Unknown machine number %u", machine);
    }
    break;

  case ASSIGNMENT_2014:
    {
      switch(machine) {
      case  0:
      case 16:
	return "M1";
      case  1:
      case 17:
	return "M2";
      case  2:
      case 18:
	return "M3";
      case  3:
      case 19:
	return "M4";
      case  4:
      case 20:
	return "M5";
      case  5:
      case 21:
	return "M6";
      case  6:
      case 22:
	return "M7";
      case  7:
      case 23:
	return "M8";
      case  8:
      case 24:
	return "M9";
      case  9:
      case 25:
	return "M10";
      case 10:
      case 26:
	return "M11";
      case 11:
      case 27:
	return "M12";
      case 12:
      case 28:
	return "D1";
      case 13:
      case 29:
	return "D2";
      case 14:
      case 30:
	return "D3";
      case 15:
      case 31:
	return "R1";
      default:
	throw fawkes::Exception("Unknown machine number %u", machine);
      }
    }
    break;
  default:
    throw fawkes::Exception("Unknown assignment/machine name");
  }
}

} // end of namespace llsfrb
