
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

#include "ssl_msgs/SslWrapper.pb.h"
#include <msgs/VisionData.pb.h>

#include <list>

namespace protobuf_comm {
  class ProtobufStreamClient;
}
namespace llsfrb {
  class Configuration;
}

namespace llsfrb_visproc {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class MachineArea{
  public:
    unsigned int width;
    unsigned int height;
    unsigned int start_x;
    unsigned int start_y;
    std::list<llsf_msgs::Pose2D *> pucks;
    std::string name;
  
  public:
    bool in_area(int x, int y);
};

class LLSFRefBoxVisionProcessor
{
 public:
  std::vector<MachineArea *> areas; 

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

  void start_ssl_recv();
  void handle_ssl_recv(const boost::system::error_code& error, size_t bytes_rcvd);

  void ssl_to_llsf_coord(float &x, float &y)
  {
    x = (x / 1000.f) + cfg_coord_offset_x_;
    y = (y / 1000.f) + cfg_coord_offset_y_;
  }

  void add_robot(llsf_msgs::VisionData &vd, const SSLDetectionRobot &robot);
  void add_puck(llsf_msgs::VisionData &vd, const SSLDetectionBall &puck);

  void process_pucks(llsf_msgs::VisionData &vd);

 private: // members
  llsfrb::Configuration *config_;

  bool        quit_;
  boost::asio::io_service      io_service_;
  boost::asio::deadline_timer  reconnect_timer_;
  bool                         try_reconnect_;

  protobuf_comm::ProtobufStreamClient *client;

  size_t in_data_size_;
  void * in_data_;
  boost::asio::ip::udp::socket   ssl_socket_;
  boost::asio::ip::udp::endpoint ssl_in_endpoint_;

  bool printed_cannot_send_;

  float cfg_coord_offset_x_;
  float cfg_coord_offset_y_;
};

} // end of namespace llsfrb_visproc

#endif
