
/***************************************************************************
 *  visproc.h - LLSF RefBox vision processor
 *
 *  Created: Fri Jun 07 14:23:36 2013
 *  Copyright  2013  Tim Niemueller [www.niemueller.de]
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

#ifndef __LLSF_REFBOX_VISION_VISPROC_H_
#define __LLSF_REFBOX_VISION_VISPROC_H_

#include <boost/asio.hpp>
#include <google/protobuf/message.h>

namespace protobuf_comm {
  class ProtobufStreamClient;
}

namespace llsfrb_visproc {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif


class LLSFRefBoxShellMachine;
class LLSFRefBoxShellRobot;
class LLSFRefBoxShellOrder;
class LLSFRefBoxShellPuck;

class LLSFRefBoxVisionProcessor
{
 public:
  LLSFRefBoxVisionProcessor();
  ~LLSFRefBoxVisionProcessor();

  int run();

 private: // methods
  void handle_reconnect_timer(const boost::system::error_code& error);
  void handle_signal(int signum);

  void client_connected();
  void client_disconnected(const boost::system::error_code &error);
  void client_msg(uint16_t comp_id, uint16_t msg_type,
		  std::shared_ptr<google::protobuf::Message> msg);

 private: // members
  bool        quit_;
  boost::asio::io_service      io_service_;
  boost::asio::deadline_timer  reconnect_timer_;
  bool                         try_reconnect_;

  protobuf_comm::ProtobufStreamClient *client;
};


} // end of namespace llsfrb_visproc

#endif
