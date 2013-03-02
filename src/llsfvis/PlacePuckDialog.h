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
#include <msgs/MachineSpec.pb.h>

namespace LLSFVis {

class PlacePuckDialog : public Gtk::Dialog{
public:
	PlacePuckDialog(std::vector<llsf_msgs::Puck*>& pucks, std::vector<llsf_msgs::Machine*>& machines);
	virtual ~PlacePuckDialog();
	llsf_msgs::Machine* get_selected_machine();
	llsf_msgs::Puck* get_selected_puck();
private:

	std::vector<llsf_msgs::Puck*>& pucks_;
	std::vector<llsf_msgs::Machine*>& machines_;
	Gtk::Grid grid_;
	Gtk::ComboBoxText combo_pucks_;
	Gtk::ComboBoxText combo_machines_;


};

} /* namespace LLSFVis */
#endif /* PLACEPUCKDIALOG_H_ */
