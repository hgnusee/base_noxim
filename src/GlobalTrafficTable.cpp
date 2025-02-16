/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the global traffic table
 */

#include "GlobalTrafficTable.h"

GlobalTrafficTable::GlobalTrafficTable()
{
}

bool GlobalTrafficTable::load(const char *fname)
{
  // Open file
  ifstream fin(fname, ios::in);
  if (!fin)
    return false;

  // Initialize variables
  traffic_table.clear();

  // Cycle reading file
  while (!fin.eof()) {
    char line[512];
    fin.getline(line, sizeof(line) - 1);

    if (line[0] != '\0') {
      if (line[0] != '%') {
	int src, dst;	// Mandatory
	double pir, por;
	int t_on, t_off, t_period;

	int params =
	  sscanf(line, "%d %d %lf %lf %d %d %d", &src, &dst, &pir,
		 &por, &t_on, &t_off, &t_period);
	if (params >= 2) {
	  // Create a communication from the parameters read on the line
	  Communication communication;

	  // Mandatory fields
	  communication.src = src;
	  communication.dst = dst;

	  // Custom PIR
	  if (params >= 3 && pir >= 0 && pir <= 1)
	    communication.pir = pir;
	  else
	    communication.pir =
	      GlobalParams::packet_injection_rate;

	  // Custom POR
	  if (params >= 4 && por >= 0 && por <= 1)
	    communication.por = por;
	  else
	    communication.por = communication.pir;	// GlobalParams::probability_of_retransmission;

	  // Custom Ton
	  if (params >= 5 && t_on >= 0)
	    communication.t_on = t_on;
	  else
	    communication.t_on = 0;

	  // Custom Toff
	  if (params >= 6 && t_off >= 0) {
	    assert(t_off > t_on);
	    communication.t_off = t_off;
	  } else
	    communication.t_off =
	      GlobalParams::reset_time +
	      GlobalParams::simulation_time;

	  // Custom Tperiod
	  if (params >= 7 && t_period > 0) {
	    assert(t_period > t_off);
	    communication.t_period = t_period;
	  } else
	    communication.t_period =
	      GlobalParams::reset_time +
	      GlobalParams::simulation_time;

	  // Add this communication to the vector of communications
	  traffic_table.push_back(communication);
	}
      }
    }
  }

  return true;
}

// HG: load Traffic Table from file, need to set TRAFFIC_COMMUNICATION_TABLE in config file
// Format of Traffic Communication File:
// src dst data_volume waitID waitOP
bool GlobalTrafficTable::loadTrafficFile(const char *fname)
{
  // Open file
  ifstream fin(fname, ios::in);
  if (!fin)
    return false;

  // Initialize variables
  traffic_communication_table.clear();
  reserved_traffic_communication_table.clear();

  // Cycle reading file
  while (!fin.eof()) {
    char line[512];
    fin.getline(line, sizeof(line) - 1);

    if (line[0] != '\0') {
      if (line[0] != '%') {
		int taskID, src, dst;	// Mandatory
		int data_vol, waitID, waitOP;

		int params =
		sscanf(line, "%d %d %d %d %d %d", &taskID, &src, &dst, &data_vol,
			&waitID, &waitOP);
		if (params >= 2 && params <= 6) {
			// Create a communication from the parameters read on the line
			TrafficCommunication TrafficCommunication;

			// Mandatory fields
			TrafficCommunication.taskID = taskID;
			TrafficCommunication.src = src;
			TrafficCommunication.dst = dst;
			TrafficCommunication.data_volume = data_vol;
			TrafficCommunication.waitID = waitID;
			TrafficCommunication.waitOP = waitOP;

			TrafficCommunication.traffic_used = false; // HG: all new traffic are 'unused'
			TrafficCommunication.trn_complete = false; // HG: all new traffic transmission are 'incomplete'
			TrafficCommunication.cmp_complete = false; // HG: all new traffic computation in dst PE are 'incomplete'

			// Bucket the Traffic into reserved_traffic_communication_table or not
			if (waitID == -1) {
				traffic_communication_table.push_back(TrafficCommunication);
			} else if (waitID >= 0) {
				// Add to reserved traffic table
				reserved_traffic_communication_table.push_back(TrafficCommunication);
			} else {
				assert("Wrong Traffic! Ensure waitID >= -1 !");
			}

		} else {
			// ensure all params must be present in traffic communication file
			assert(params == 6);
		}
      }
    }
  }

  return true;
}

double GlobalTrafficTable::getCumulativePirPor(const int src_id,const int ccycle, const bool pir_not_por,vector < pair < int, double > > &dst_prob)
{
  double cpirnpor = 0.0;

  dst_prob.clear();

  for (unsigned int i = 0; i < traffic_table.size(); i++) {
    Communication comm = traffic_table[i];
    if (comm.src == src_id) {
      int r_ccycle = ccycle % comm.t_period;
      if (r_ccycle > comm.t_on && r_ccycle < comm.t_off) {
		cpirnpor += pir_not_por ? comm.pir : comm.por;
		pair < int, double >dp(comm.dst, cpirnpor);
		dst_prob.push_back(dp);
      }
    }
  }

  return cpirnpor;
}

TrafficCommunication& GlobalTrafficTable::getTrafficCommunicationTable(const int src_id)
{
  for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {

	if (traffic_communication_table[i].src == src_id) {
		// remove transaction from transaction communication table once used
		// cout << "DEBUG: Traffic Communication Table found for src_id = " << src_id << endl;
		// traffic_communication_table[i].traffic_used = true; // this flag is done in PE canShot() function
		// return transaction to Processing Element to make packet
	  return traffic_communication_table[i];
	}
  }

  // HG:return empty TrafficCommunication, if not matching src_id found
//   cout << "DEBUG: No Traffic Communication Table found for src_id = " << src_id << endl;
  return empty_comm;
}

void GlobalTrafficTable::moveReserveToTrafficCommunicationTable(const int src_id) {
	/*
	cout << "DEBUG: Print Reserve Table before MOVING" << endl;
	// Print reserved_traffic_communication_table
	for (const auto& comm : reserved_traffic_communication_table) {
		cout << "src: " << comm.src << ", dst: " << comm.dst << ", data_volume: " << comm.data_volume
				<< ", waitID: " << comm.waitID << ", waitOP: " << comm.waitOP << endl;
	}
	*/
	// find all reserved transactions for matching src_id
	//   and move the matching ones to compare with traff comm table

	// vector <TrafficCommunication> reserved_comm = {};
	vector <unsigned int> index_to_remove;

	for (unsigned int i = 0; i < reserved_traffic_communication_table.size(); i++) {
		TrafficCommunication reserved_comm = reserved_traffic_communication_table[i];
		if (reserved_comm.src == src_id) {
			// reserved_comm.push_back(reserved_comm);
			cout << "DEBUG: Found Reserved Traffic for src_id = " << src_id << endl;

			// loop and compare with traffic_communication_table
			for (unsigned int j = 0; j < traffic_communication_table.size(); j++) {
				TrafficCommunication tcomm = traffic_communication_table[j];

                if (reserved_comm.waitID == tcomm.taskID) {
                    if ((reserved_comm.waitOP == CMP && tcomm.cmp_complete) ||
                        (reserved_comm.waitOP == TRN && tcomm.trn_complete)) {
                        traffic_communication_table.push_back(reserved_comm);
                        index_to_remove.push_back(i);
                        break;
                    }
                }
			}
		}
	}
	// Erase elements in reverse order to avoid index shifting issues
    for (auto it = index_to_remove.rbegin(); it != index_to_remove.rend(); ++it) {
        reserved_traffic_communication_table.erase(reserved_traffic_communication_table.begin() + *it);
    }

	// the reserved_comm should compare with traffic_communication_table
	// 	using condition: reserved_comm.waitID == traffic_communication_table.taskID && reserved_comm.waitOP == TRANS|COMP COMPLETE

}

void GlobalTrafficTable::setTransmitComplete(const int task_ID) {

	for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
		TrafficCommunication comm = traffic_communication_table[i];
		if (comm.taskID == task_ID) {
			traffic_communication_table[i].trn_complete = true;
			// logic to check that cmp_complete should be false, since transmission is done first before compute can be done
			assert(traffic_communication_table[i].cmp_complete == false);
			break;
		}
	}
}

void GlobalTrafficTable::setComputeComplete(const int task_ID) {

	for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
		// TrafficCommunication comm = traffic_communication_table[i];
		if (traffic_communication_table[i].taskID == task_ID) {
			traffic_communication_table[i].cmp_complete = true;
			// logic to check that trn_complete is also true, since transmission is done first before compute can be done
			assert(traffic_communication_table[i].trn_complete == true);
			break;
		}
	}
}


int GlobalTrafficTable::occurrencesAsSource(const int src_id)
{
  int count = 0;

  for (unsigned int i = 0; i < traffic_table.size(); i++)
    if (traffic_table[i].src == src_id)
      count++;

  return count;
}
