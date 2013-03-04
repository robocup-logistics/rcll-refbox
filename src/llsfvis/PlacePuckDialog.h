/*
 * PlacePuckDialog.h
 *
 *  Created on: 02.03.2013
 *      Author: daniel
 */

#ifndef PLACEPUCKDIALOG_H_
#define PLACEPUCKDIALOG_H_

#include <gtkmm.h>
#include <msgs/PuckInfo.pb.h>
#include <msgs/MachineInfo.pb.h>

namespace LLSFVis {

class PlacePuckDialog : public Gtk::Dialog{
public:
	PlacePuckDialog(const llsf_msgs::PuckInfo& pucks, const llsf_msgs::MachineInfo& machines);
	virtual ~PlacePuckDialog();
	const llsf_msgs::Machine& get_selected_machine();
	const llsf_msgs::Puck& get_selected_puck();
private:

	const llsf_msgs::PuckInfo& pucks_;
	const llsf_msgs::MachineInfo& machines_;
	Gtk::Grid grid_;
	Gtk::ComboBoxText combo_pucks_;
	Gtk::ComboBoxText combo_machines_;


};

} /* namespace LLSFVis */
#endif /* PLACEPUCKDIALOG_H_ */
