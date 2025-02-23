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
		int taskID;	// Mandatory
		int data_vol, waitID, waitOP;

		char src_str[512], dst_str[512];

		int params =
		sscanf(line, "%d [%[^]]] [%[^]]] %d %d %d", &taskID, src_str, dst_str, &data_vol,
			&waitID, &waitOP);
		if (params == 6) {
			// parsing src_str to vector<int> src
			vector<int> src;
			char *token = strtok(src_str, " ");
			// HG: additional work to parse src_str to vector<int> src
			while (token != NULL) {
				src.push_back(atoi(token));
				token = strtok(NULL, " ");
			}

			// Parse dst vector
			vector<int> dst;
			token = strtok(dst_str, " ");
			while (token != NULL) {
				dst.push_back(atoi(token));
				token = strtok(NULL, " ");
			}

			// Validate src/dst vector rule
			if (!((src.size() == 1 && dst.size() >= 1) || 
					(src.size() > 1 && dst.size() == 1))) {
				cerr << "Error: Either src or dst must be size 1, while the other can be >1" << endl;
				assert(false);
			}

			// Create a communication from the parameters read on the line
			TrafficCommunication TrafficCommunication;

			// Mandatory fields
			TrafficCommunication.taskID = taskID;
			TrafficCommunication.src = src; // HG: src is now a vector
			TrafficCommunication.dst = dst;
			TrafficCommunication.data_volume = data_vol;
			TrafficCommunication.waitID = waitID;
			TrafficCommunication.waitOP = waitOP;

			TrafficCommunication.traffic_used = false; // HG: all new traffic are 'unused'
			// Initialize vectors of trn_complete and cmp_complete with 0, matching size of src/dst (whichever larger)
			size_t size_to_use = max(src.size(), dst.size());
			TrafficCommunication.trn_complete.resize(size_to_use, TRN_WAIT);
			TrafficCommunication.cmp_complete.resize(size_to_use, CMP_WAIT);

			// Bucket the Traffic into reserved_traffic_communication_table or not
			if (waitID == -1) {
				traffic_communication_table.push_back(TrafficCommunication);
			} else if (waitID >= 0) {
				// Add to reserved traffic table
				reserved_traffic_communication_table.push_back(TrafficCommunication);
			} else {
				assert(false && "Wrong Traffic! Ensure waitID >= -1 !");
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
	
	// To accomadate vector of src, use find() function
	bool found_src = false;
	bool found_dst = false;
	size_t src_pos = 0;
	size_t dst_pos = 0;
	int check_trn_state = TRN_WAIT;  // default state

	// store in tcomm variable for readability
	TrafficCommunication& tcomm = traffic_communication_table[i];
	auto it = find(tcomm.src.begin(), tcomm.src.end(), src_id);
	found_src = (it != tcomm.src.end());

	if (tcomm.src.size() > 1 && tcomm.dst.size() == 1) {

		if (found_src) {
			src_pos = distance(tcomm.src.begin(), it);
			check_trn_state = tcomm.trn_complete[src_pos];

			if (check_trn_state == TRN_WAIT) {

				cout << "DEBUG: Traffic Comm Table found for src_id = " << src_id << endl;
				// return transaction to Processing Element to make packet
				tcomm.trn_complete[src_pos] = TRN_BUSY;
		
			  return tcomm;
		
			} else if (check_trn_state != TRN_WAIT) {
		
				cout << "DEBUG: Traffic Comm Table found for src_id = " << src_id 
					<< " but trn_complete is not WAIT" << endl;
			}
		} else {
			continue;
		}

	} else if ((tcomm.src.size() == 1 && tcomm.dst.size() > 1) || 
					(tcomm.src.size() == 1 && tcomm.dst.size() == 1)) {

		// One-to-One OR One-to-Many case: check if dst_id is being tagged or not
		// 	in the tcomm trn_complete vector
		// if trn_complete is TRN_WAIT, then tag as TRN_BUSY & return tcomm
		// 	else, print debugg message and continue with loop

		auto it = find(tcomm.trn_complete.begin(), tcomm.trn_complete.end(), TRN_WAIT);
		found_dst = (it != tcomm.trn_complete.end());
		dst_pos = distance(tcomm.trn_complete.begin(), it);

		// return tcomm with dst found with TRN_WAIT
		// actually dont need to check == TRN_WAIT anymore but for clarity
		if (found_src == true && found_dst == true && tcomm.trn_complete[dst_pos] == TRN_WAIT) {

			// tcomm.trn_complete[dst_pos] = TRN_BUSY;

			return tcomm;
		} else {
			continue;
		}

	} else {
		cerr << "Error: Only one of src OR dst can > 1" << endl;
		assert(false);
	}

  }

  	// HG: return empty TrafficCommunication, if not matching src_id found
	//   cout << "DEBUG: No Traffic Communication Table found for src_id = " << src_id << endl;
  return empty_comm;
}

void GlobalTrafficTable::moveReserveToTrafficCommunicationTable(const int src_id) {

	vector <unsigned int> index_to_remove;

	for (unsigned int i = 0; i < reserved_traffic_communication_table.size(); i++) {
		TrafficCommunication reserved_comm = reserved_traffic_communication_table[i];
		auto src_it = find(reserved_comm.src.begin(), reserved_comm.src.end(), src_id);

		if (src_it != reserved_comm.src.end()){

			cout << "DEBUG: Found Reserved Traffic for src_id = " << src_id << endl;

			// loop and compare with traffic_communication_table
			for (unsigned int j = 0; j < traffic_communication_table.size(); j++) {
				TrafficCommunication tcomm = traffic_communication_table[j];

				// Check if all elements in trn_complete & cmp_complete match their respective completion states
				bool all_trn_done = true;
				bool all_cmp_done = true;

				for (size_t k = 0; k < tcomm.trn_complete.size(); k++) {
					if (tcomm.trn_complete[k] != TRN_DONE) all_trn_done = false;
					if (tcomm.cmp_complete[k] != CMP_DONE) all_cmp_done = false;
				}

				// If both vectors have all elements in completed state
                if (reserved_comm.waitID == tcomm.taskID) {
                    if ((reserved_comm.waitOP == CMP && all_cmp_done) ||
                        (reserved_comm.waitOP == TRN && all_trn_done)) {
                        traffic_communication_table.push_back(reserved_comm);
                        index_to_remove.push_back(i);
                        break;
                    }
                }
			}
		} else {
			continue;
		}
	}
	// Erase elements in reverse order to avoid index shifting issues
    for (auto it = index_to_remove.rbegin(); it != index_to_remove.rend(); ++it) {
        reserved_traffic_communication_table.erase(reserved_traffic_communication_table.begin() + *it);
    }

}

void GlobalTrafficTable::setTransmitComplete(const int task_ID, const int src_ID, const int dst_ID) {

	for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
		TrafficCommunication& comm = traffic_communication_table[i];

		if (comm.taskID == task_ID) {
			
			// Consider One-to-Many OR Many-to-One OR one-to-one case
			if (comm.src.size() == 1 && comm.dst.size() >= 1) {
				// Find position of dst_ID in the src vector
				auto it = find(comm.dst.begin(), comm.dst.end(), dst_ID);
				if (it != comm.dst.end()) {
					// Calculate position
					size_t pos = distance(comm.dst.begin(), it);
					// Set trn_complete at found position to TRN_DONE
					comm.trn_complete[pos] = TRN_DONE;
				}

			} else if (comm.src.size() >= 1 && comm.dst.size() == 1) {
				// Find position of src_ID in the src vector
				auto it = find(comm.src.begin(), comm.src.end(), src_ID);
				if (it != comm.src.end()) {
					// Calculate position
					size_t pos = distance(comm.src.begin(), it);
					// Set trn_complete at found position to TRN_DONE
					comm.trn_complete[pos] = TRN_DONE;
				}

			}

			// Check if all trn_complete values are TRN_DONE
			bool all_done = true;
			for (const auto& trn_status : comm.trn_complete) {
				if (trn_status != TRN_DONE) {
					all_done = false;
					break;
				}
			}
			if (all_done) {
				cout << "DEBUG: All traffic complete for taskID = " << task_ID << endl;
				comm.traffic_used = true;
			}
			break;

		} else {
			continue;
		}
	}
}

void GlobalTrafficTable::setComputeComplete(const int task_ID, const int src_ID, const int local_ID) {

	for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
		TrafficCommunication& comm = traffic_communication_table[i];

		if (comm.taskID == task_ID) {

			// Consider One-to-Many OR Many-to-One OR one-to-one case
			if (comm.src.size() == 1 && comm.dst.size() > 1) {
				// Find position based on PE local_ID (aka dst vector)
				auto it = find(comm.dst.begin(), comm.dst.end(), local_ID);
				if (it != comm.dst.end()) {
					// Calculate position
					size_t pos = distance(comm.dst.begin(), it);
					// Set trn_complete at found position to CMP_DONE
					comm.cmp_complete[pos] = CMP_DONE;
				}
			}
			else if (comm.src.size() >= 1 && comm.dst.size() == 1) {
				// Find position of src_ID in the src vector
				auto it = find(comm.src.begin(), comm.src.end(), src_ID);
				if (it != comm.src.end()) {
					// Calculate position
					size_t pos = distance(comm.src.begin(), it);
					// Set trn_complete at found position to CMP_DONE
					comm.cmp_complete[pos] = CMP_DONE;
				}
			}
			// Check if all trn_complete values are TRN_DONE
			bool all_done = true;
			for (const auto& cmp_status : comm.cmp_complete) {
				if (cmp_status != CMP_DONE) {
					all_done = false;
					break;
				}
			}
			if (all_done) {
				cout << "DEBUG: All ComputeProcess() complete for taskID = " << task_ID << endl;
				// comm.traffic_used = true;
			}
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
