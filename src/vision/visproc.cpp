
/***************************************************************************
 *  visproc.cpp - LLSF RefBox vision processor
 *
 *  Created: Fri Jun 07 14:26:28 2013
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

#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

#include "visproc.h"

#include <protobuf_comm/client.h>
#include <config/yaml.h>

#include <msgs/MachineInfo.pb.h>
#include <libs/utils/hungarian_method/hungarian.h>

#include <csignal>
#include <cmath>

// defined in miliseconds
#define RECONNECT_TIMER_INTERVAL 1000
#define MAX_PACKET_LENGTH 65536

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

namespace llsfrb_visproc {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#if BOOST_ASIO_VERSION < 100601
static bool g_quit = false;
#endif

bool MachineArea::in_area(unsigned int x, unsigned int y, unsigned int tol) {
  if( (x >= start_x - tol &&
       x <= start_x + tol)
      &&
      (y >= start_y - tol &&
       y <= start_y + tol)) {
    return true;
  }
  return false;
}

bool MachineArea::apply_hungarian() {
  std::vector<llsf_msgs::VisionObject *> tmp_pucks;
  hungarian_problem_t hp;
  std::vector<unsigned int> obj_ids(pucks.size());
  hp.num_rows = new_pucks.size();
  hp.num_cols = pucks.size();
  hp.cost = (int**) calloc(hp.num_rows, sizeof(int*));
  for (int i = 0; i < hp.num_rows; i++)
    hp.cost[i] = (int*) calloc(hp.num_cols, sizeof(int));
  for (int row = 0; row < hp.num_rows; row++) { // new centroids
    unsigned int col = 0;
    for (std::vector<llsf_msgs::VisionObject *>::iterator col_it = pucks.begin(); col_it != pucks.end(); col_it++, col++) {
      Pose2D old_pose = (*col_it)->pose();
      Pose2D new_pose = new_pucks[row]->pose();
      //double distance = ( old_pose.x()-new_pose.x() ) * ( old_pose.x()-new_pose.x() )
      //                + ( old_pose.y()-new_pose.y() ) * ( old_pose.y()-new_pose.y() );
      //distance = sqrt(distance);
      double dist = distance( old_pose, new_pose );
      hp.cost[row][col] = (int)(dist);
    }
  }  
 
  HungarianMethod solver;
  solver.init(hp.cost, hp.num_rows, hp.num_cols, HUNGARIAN_MODE_MINIMIZE_COST);
  solver.solve();

  int assignment_size;
  int *assignment = solver.get_assignment(assignment_size);
  unsigned int id;
  for( int row = 0; row < assignment_size; row++) {
    if( row >= hp.num_rows ) {  //object disappeared
      id = pucks[assignment[row]]->id();
      old_pucks.push_back(pucks[row]);
      continue;
    }
    else if (assignment[row] >= hp.num_cols ) { //object is new or has reappeared
      bool assigned = false;
      for( std::vector<llsf_msgs::VisionObject *>::iterator it = old_pucks.begin(); it != old_pucks.end(); it++ ) {
        if( distance(new_pucks[row]->pose(), (*it)->pose()) <= 5 ) { //check distance
          id = (*it)->id();
          old_pucks.erase(it);
          assigned = true;
          break;
        }
      }
      if( !assigned ) { continue }
    }
    else { 
      id = pucks[assignment]->id();
      if ( distance(pucks[id]->pose(), new_pucks[row]) <= 5 ) {
        old_pucks.push_back(pucks[id]);
        continue;
      }
    }
    tmp_pucks[id] = new_pucks[row];
  }   
  



  return true; 
}    

double MachineArea::distance(float x1, float x2, float y1, float y2) {
  return sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) );
}

double MachineArea::distance(const llsf_msgs::Pose2D &a, const llsf_msgs::Pose2D &b) {
  return sqrt( (a.x() - b.x())*(a.x() - b.x()) + (a.y() - b.y())*(a.y() - b.y()) );
}

LLSFRefBoxVisionProcessor::LLSFRefBoxVisionProcessor()
  : quit_(false), reconnect_timer_(io_service_), try_reconnect_(true),
    ssl_socket_(io_service_), printed_cannot_send_(false)
{
  client = new ProtobufStreamClient();
  in_data_size_ = MAX_PACKET_LENGTH;
  in_data_ = malloc(in_data_size_);
  config_ = new llsfrb::YamlConfiguration(CONFDIR);
  config_->load("config.yaml");
}


LLSFRefBoxVisionProcessor::~LLSFRefBoxVisionProcessor()
{
  quit_ = true;
  io_service_.stop();
  try_reconnect_ = false;
      
  reconnect_timer_.cancel();

  ssl_socket_.close();
  free(in_data_);

  delete client;
  client = 0;

  delete config_;
}


/** Handle operating system signal.
 * @param error error code
 * @param signum signal number
 */
void
LLSFRefBoxVisionProcessor::handle_signal(int signum)
{
  reconnect_timer_.cancel();
  io_service_.stop();
}


/** Handle reconnect timer event.
 * @param error error code
 */
void
LLSFRefBoxVisionProcessor::handle_reconnect_timer(const boost::system::error_code& error)
{
  if (! error && try_reconnect_ && ! quit_) {
    client->async_connect("localhost", 4444);
  }
}

void
LLSFRefBoxVisionProcessor::client_connected()
{
  // put code here for what to do when connected to refbox
  printf("%sconnected to refbox...\n", printed_cannot_send_ ? "\n" : "");
  printed_cannot_send_ = false;
}

void
LLSFRefBoxVisionProcessor::client_disconnected(const boost::system::error_code &error)
{
  // automatically try to reconnect
  if (try_reconnect_) {
    reconnect_timer_.expires_from_now(boost::posix_time::milliseconds(RECONNECT_TIMER_INTERVAL));
    reconnect_timer_.async_wait(boost::bind(&LLSFRefBoxVisionProcessor::handle_reconnect_timer, this,
					    boost::asio::placeholders::error));
  }
}

void
LLSFRefBoxVisionProcessor::client_msg(uint16_t comp_id, uint16_t msg_type,
				      std::shared_ptr<google::protobuf::Message> msg)
{
  if ( areas.empty() ) {
  printf("Received machine info for the first time\n");
  std::shared_ptr<llsf_msgs::MachineInfo> minfo;
    if ((minfo = std::dynamic_pointer_cast<llsf_msgs::MachineInfo>(msg))) {
      // use machine info
      for( int i = 0; i < minfo->machines_size(); i++ ) {
        MachineArea * new_area = new MachineArea;
        Machine m = minfo->machines(i);
        std::string type = m.type();
        if ( type[0] == 'T') {
          new_area->name = m.name();
          Pose2D p = m.pose();
          int x = p.x() * 1000;
          int y = p.y() * 1000;
          new_area->width = config_->get_uint("/llsfrb/visproc/machine-area-width");
          new_area->height = config_->get_uint("/llsfrb/visproc/machine-area-height");
          new_area->start_x = x - (new_area->width)/2;
          new_area->start_y = y - (new_area->height)/2;
          areas.push_back(new_area);
        }
      delete new_area;
      }
    }
  }
}

#if BOOST_ASIO_VERSION < 100601
static void old_handle_signal(int signum)
{
  g_quit = true;
}
#endif


void
LLSFRefBoxVisionProcessor::add_robot(llsf_msgs::VisionData &vd, const SSLDetectionRobot &robot)
{
  if (! robot.has_robot_id()) {
    printf("Received robot without ID from SSL Vision, ignoring");
    return;
  }

  VisionObject *r = vd.add_robots();
  r->set_id(robot.robot_id());
  r->set_confidence(robot.confidence());
  Pose2D *pose = r->mutable_pose();
  float x = robot.x();
  float y = robot.y();
  ssl_to_llsf_coord(x, y);
  pose->set_x(x);
  pose->set_y(y);
  pose->set_ori(robot.orientation());

  printf("Robot %u @ (%f,%f,%f)\n", robot.robot_id(), x, y, robot.orientation());

  struct timespec now;;
  clock_gettime(CLOCK_REALTIME, &now);
  Time *pose_t = pose->mutable_timestamp();
  pose_t->set_sec(now.tv_sec);
  pose_t->set_nsec(now.tv_nsec);
}

void
LLSFRefBoxVisionProcessor::add_puck(llsf_msgs::VisionData &vd, const SSLDetectionBall &puck)
{
  VisionObject *r = vd.add_pucks();
  r->set_id(0);
  r->set_confidence(puck.confidence());
  Pose2D *pose = r->mutable_pose();
  float x = puck.x();
  float y = puck.y();
  ssl_to_llsf_coord(x, y);
  pose->set_x(x);
  pose->set_y(y);
  pose->set_ori(0);

  printf("Puck @ (%f,%f)\n", x, y);

  struct timespec now;;
  clock_gettime(CLOCK_REALTIME, &now);
  Time *pose_t = pose->mutable_timestamp();
  pose_t->set_sec(now.tv_sec);
  pose_t->set_nsec(now.tv_nsec);
}

void
LLSFRefBoxVisionProcessor::process_pucks() {
  if( !areas.empty() ) {
    while( !ssl_pucklist.empty() ) {
      VisionObject * p = ssl_pucklist.back();
      ssl_pucklist.pop_back();
      Pose2D pose = p->pose();
      unsigned int x = (unsigned int)pose.x();
      unsigned int y = (unsigned int)pose.y();

      for( unsigned int i = 0; i < areas.size(); i++ ) {
        if( areas[i]->in_area(x, y, 10) ) {
          areas[i]->new_pucks.push_back(p);
        }

      }
    }
  for( unsigned int i = 0; i < areas.size(); i++ ) {
    areas[i]->apply_hungarian();
  }
  }
}

void
LLSFRefBoxVisionProcessor::create_pucklist(const SSLDetectionBall &puck, unsigned int id) {
  VisionObject *p = new VisionObject;
  
  p->set_id(id);
  p->set_confidence(puck.confidence());

  Pose2D *pose = p->mutable_pose();
  float x = puck.x();
  float y = puck.y();
  ssl_to_llsf_coord(x, y);
  pose->set_x(x);
  pose->set_y(y);
  pose->set_ori(0);

  struct timespec now;;
  clock_gettime(CLOCK_REALTIME, &now);
  Time *pose_t = pose->mutable_timestamp();
  pose_t->set_sec(now.tv_sec);
  pose_t->set_nsec(now.tv_nsec);
  
  ssl_pucklist.push_back(p);
  delete p;
}  

void
LLSFRefBoxVisionProcessor::handle_ssl_recv(const boost::system::error_code& error,
					   size_t bytes_rcvd)
{
  if (! error) {
    SSLWrapperPacket packet;
    packet.ParseFromArray(in_data_, bytes_rcvd);

    llsf_msgs::VisionData vd;

    struct timespec now;;
    clock_gettime(CLOCK_REALTIME, &now);
    Time *pose_t = vd.mutable_time();
    pose_t->set_sec(now.tv_sec);
    pose_t->set_nsec(now.tv_nsec);

    ssl_pucklist.clear();

    if (packet.has_detection()) {
      const SSLDetectionFrame &detection = packet.detection();
      // pucks
	/*
      const int number_pucks = detection.balls_size();
      for (int i = 0; i < number_pucks; ++i) {
	const SSLDetectionBall &ball = detection.balls(i);
      
	VisionObject *p = vd.add_pucks();
	p->set_id(i);
	p->set_confidence(ball.confidence());
      
	Pose2D *pose = p->mutable_pose();
	pose->set_x(ball.x());
	pose->set_y(ball.y());
	pose->set_ori(0);
	Time *t = pose->mutable_timestamp();
	t->set_sec(123456);
	t->set_nsec(654);
      
	t = msg->mutable_time();
	t->set_sec(123456);
	t->set_nsec(654);
      }
	*/

      for (int i = 0; i < detection.robots_blue_size(); ++i) {
	    add_robot(vd, detection.robots_blue(i));
      }
      for (int i = 0; i < detection.robots_yellow_size(); ++i) {
	    add_robot(vd, detection.robots_yellow(i));
      }
    
      for (int i = 0; i < detection.balls_size(); ++i) {
        //add_puck(vd, detection.balls(i));
        create_pucklist(detection.balls(i),i);
      }
    }
    
    if ( !ssl_pucklist.empty() ) {
      process_pucks();
    }
 
    if (vd.pucks_size() > 0 || vd.robots_size() > 0) {
      try {
	client->send(vd);
      } catch (std::runtime_error &e) {
	if (printed_cannot_send_) {
	  printf(".");
	} else {
	  printf("Sending vision data failed: %s...", e.what());
	  printed_cannot_send_ = true;
	}
	fflush(stdout);
      }
    }
  }

  start_ssl_recv();
}


void
LLSFRefBoxVisionProcessor::start_ssl_recv()
{
  ssl_socket_.async_receive_from(boost::asio::buffer(in_data_, in_data_size_),
				 ssl_in_endpoint_,
				 boost::bind(&LLSFRefBoxVisionProcessor::handle_ssl_recv,
					     this, boost::asio::placeholders::error,
					     boost::asio::placeholders::bytes_transferred));
}

int
LLSFRefBoxVisionProcessor::run()
{
  // Add messages you want to receive and process
  //MessageRegister & message_register = client->message_register();
  //message_register.add_message_type<llsf_msgs::GameState>();

  cfg_coord_offset_x_ = config_->get_float("/llsfrb/visproc/coord-offset-x");
  cfg_coord_offset_y_ = config_->get_float("/llsfrb/visproc/coord-offset-y");

  std::string  listen_address = config_->get_string("/llsfrb/visproc/listen-address");
  std::string  multicast_addr = config_->get_string("/llsfrb/visproc/ssl-multicast-addr");
  unsigned int multicast_port = config_->get_uint("/llsfrb/visproc/ssl-multicast-port");
  
  boost::asio::ip::udp::endpoint
    listen_endpoint(boost::asio::ip::address::from_string(listen_address), multicast_port);
  ssl_socket_.open(listen_endpoint.protocol());
  ssl_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
  ssl_socket_.bind(listen_endpoint);

  boost::asio::ip::address mc_addr =
    boost::asio::ip::address::from_string(multicast_addr);
  ssl_socket_.set_option(boost::asio::ip::multicast::join_group(mc_addr));

  start_ssl_recv();

  // Connect signals for network client
  client->signal_connected().connect(
    boost::bind(&LLSFRefBoxVisionProcessor::client_connected, this));
  client->signal_disconnected().connect(
    boost::bind(&LLSFRefBoxVisionProcessor::client_disconnected,
		this, boost::asio::placeholders::error));
  client->signal_received().connect(
    boost::bind(&LLSFRefBoxVisionProcessor::client_msg, this, _1, _2, _3));

  // Initial connect, will retry until connected asynchronously!
  client->async_connect("localhost", 4444);

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(boost::bind(&LLSFRefBoxVisionProcessor::handle_signal, this,
				 boost::asio::placeholders::signal_number));
#else
  signal(SIGINT, old_handle_signal);
  signal(SIGTERM, old_handle_signal);
  while (! g_quit) {
    // Run the I/O main loop, the rest happens in the callbacks
    io_service_.run();
    usleep(500000);
  }
#endif

  // Run the I/O main loop, the rest happens in the callbacks
  io_service_.run();

  ssl_socket_.set_option(boost::asio::ip::multicast::leave_group(mc_addr));

  return 0;
}

} // end of namespace llsfrb_visproc

