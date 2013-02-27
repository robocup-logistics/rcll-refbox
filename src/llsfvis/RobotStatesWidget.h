/*
 * RobotStatesWidget.h
 *
 *  Created on: 11.02.2013
 *      Author: daniel
 */

#ifndef ROBOTSTATESWIDGET_H_
#define ROBOTSTATESWIDGET_H_

#include <gtkmm.h>
#include <msgs/RobotInfo.pb.h>
namespace LLSFVis {

class RobotStatesWidget: public Gtk::Frame {
public:
	RobotStatesWidget();
	virtual ~RobotStatesWidget();
	void setBot1(const llsf_msgs::Robot& bot);
	void setBot2(const llsf_msgs::Robot& bot);
	void setBot3(const llsf_msgs::Robot& bot);
	void clear();

private:
	bool on_update();

	const llsf_msgs::Robot* bot1_;
	const llsf_msgs::Robot* bot2_;
	const llsf_msgs::Robot* bot3_;

	Gtk::Box box_;
	Gtk::Frame bot1_frame_;
	Gtk::Label bot1_label_;
	Gtk::Frame bot2_frame_;
	Gtk::Label bot2_label_;
	Gtk::Frame bot3_frame_;
	Gtk::Label bot3_label_;

	void update_bot(time_t rawtime, const llsf_msgs::Robot* bot,Gtk::Label* bot_label);

	static const int TIMEEXCEEDED = 30;
	static const int TIMEWARNING = 5;
};

} /* namespace LLSFVis */
#endif /* ROBOTSTATESWIDGET_H_ */
