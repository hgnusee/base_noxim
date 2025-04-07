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
	empty_comm.taskID = -1;
	empty_comm.src.clear();
	empty_comm.dst.clear();
	empty_comm.src_totalVol = 0;
	empty_comm.waitID.clear();
	empty_comm.waitOP = 0;
	empty_comm.traffic_used = true;

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
//   reserved_traffic_communication_table.clear(); // not used anymore

  // Cycle reading file
  while (!fin.eof()) {
    char line[512];
    fin.getline(line, sizeof(line) - 1);

    if (line[0] != '\0') {
      if (line[0] != '%') {
		int taskID, layerID;	// Mandatory
		int src_minVol, src_totalVol, dst_minVol, dst_totalVol, waitOP;
		char traffic_type_str[4] = ""; // u, m, b

		char src_str[512], dst_str[512], waitID_str[512], nextID_str[512];

		int params =
		sscanf(line, "%d %d [%[^]]] [%[^]]] %d %d %d %d [%[^]]] [%[^]]] %d %s", 
			&taskID, &layerID, src_str, dst_str, &src_minVol,
			&src_totalVol, &dst_minVol, &dst_totalVol, waitID_str, nextID_str, &waitOP, 
			traffic_type_str);
		if (params >= 11) {
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
			// if (!((src.size() == 1 && dst.size() >= 1) || 
			// 		(src.size() > 1 && dst.size() == 1))) {
			// 	cerr << "Error: Either src or dst must be size 1, while the other can be >1" << endl;
			// 	assert(false);
			// }
			// Allow src/dst to be mutliple values for many to many case
			if (src.size() > 1 && dst.size() > 1) {
				cout << "INFO: Detected Many to Many Traffic at taskID = "<< taskID << endl;
			}

			// Parse waitID vector
			vector<int> waitID;
			token = strtok(waitID_str, " ");
			while (token != NULL) {
				waitID.push_back(atoi(token));
				token = strtok(NULL, " ");
			}

			// Parse nextID vector
			vector<int> nextID;
			token = strtok(nextID_str, " ");
			while (token != NULL) {
				nextID.push_back(atoi(token));
				token = strtok(NULL, " ");
			}
			
			// Check if waitID contains -1, it must not have more than 1 element
			if (find(waitID.begin(), waitID.end(), -1) != waitID.end() && waitID.size() > 1) {
				cerr << "Error: If waitID contains -1, it must not have more than 1 element" << endl;
				assert(false);
			}
			

			// Create a communication from the parameters read on the line
			TrafficCommunication TrafficCommunication;

			// Mandatory fields
			TrafficCommunication.layerID = layerID;
			TrafficCommunication.taskID = taskID;
			TrafficCommunication.src = src; // HG: src is now a vector
			TrafficCommunication.dst = dst;
			// Convert byte-based volumes to data_volume units
			TrafficCommunication.src_minVol = bytesToDataVolume(src_minVol);
			TrafficCommunication.src_totalVol = bytesToDataVolume(src_totalVol);
			TrafficCommunication.dst_minVol = bytesToDataVolume(dst_minVol);
			TrafficCommunication.dst_totalVol = bytesToDataVolume(dst_totalVol);
			TrafficCommunication.waitID = waitID;
			TrafficCommunication.waitOP = waitOP;
			TrafficCommunication.nextID = nextID;

			TrafficCommunication.traffic_used = false; // HG: all new traffic are 'unused'
			TrafficCommunication.compute_used = false; // HG: all new traffic initial compute status is 'false'
			// Initialize vectors of trn_complete and cmp_complete with 0, matching size of src/dst (whichever larger)
			/*size_t size_to_use = max(src.size(), dst.size());
			TrafficCommunication.trn_complete.resize(size_to_use, TRN_WAIT);
			TrafficCommunication.cmp_complete.resize(size_to_use, CMP_WAIT);
			*/
			size_t size_to_use = src.size() * dst.size();
			TrafficCommunication.trn_complete.resize(size_to_use, TRN_WAIT);
			TrafficCommunication.cmp_complete.resize(size_to_use, TRN_WAIT);

            // Check for special traffic types ###### Fri Apr 4 14:06:45 SGT 2025
            if (src.size() == 1 && dst.size() == 1 && src[0] == dst[0]) {
                TrafficCommunication.is_self_compute = true;
            } else {
                TrafficCommunication.is_self_compute = false;
            }
            
    
            // Initialize reception tracking array in parallel to trn_complete ###### Fri Apr 4 14:06:50 SGT 2025
            TrafficCommunication.rcv_complete.resize(TrafficCommunication.trn_complete.size(), RCV_WAIT);
            TrafficCommunication.traffic_received = false;
			
            // Handle traffic_type - check if the parameter was provided (12 params total)
            if (params < 12 || traffic_type_str[0] == '\0') {
                // Default to unicast if not specified or empty
                TrafficCommunication.traffic_type = T_UNICAST;
            } else {
                if (strcmp(traffic_type_str, "u") == 0)
                    TrafficCommunication.traffic_type = T_UNICAST;
                else if (strcmp(traffic_type_str, "m") == 0) {
					TrafficCommunication.traffic_type = T_MULTICAST;
					cout << "Multicast traffic recorded" << endl;
				}
                else if (strcmp(traffic_type_str, "b") == 0)
                    TrafficCommunication.traffic_type = T_BROADCAST;
                else if (strcmp(traffic_type_str, "w") == 0) {
                    // NOTE: temporary fix to allow waiting traffic types
                    TrafficCommunication.traffic_type = T_WAIT;
                    cout << "Wait traffic recorded" << endl;
                }
                else {
                    cerr << "Error parsing traffic file " << fname  
                         << ": invalid traffic_type '" << traffic_type_str 
                         << "'. Must be 'u', 'm', 'b', or 'w'" << endl;
                    assert(false);
                    return false;
                }
            }

            // Check for reception-dependent traffic ###### Fri Apr 4 14:06:47 SGT 2025

            if (src_minVol == -1 && dst_minVol == -1) {
                TrafficCommunication.is_wait_reception = true;
            } else {
                TrafficCommunication.is_wait_reception = false;
            }

			// All traffic goes to main comm table, since pipeline model decides who to go next
			if (waitID[0] >= -1) {
				traffic_communication_table.push_back(TrafficCommunication);
			// } else if (all_of(waitID.begin(), waitID.end(), [](int id) { return id >= 0; })) {
			// 	// Add to reserved traffic table
			// 	reserved_traffic_communication_table.push_back(TrafficCommunication);
			} else {
				assert(false && "Wrong Traffic! Ensure waitID >= -1 !");
			}

		} else {
			// ensure all params must be present in traffic communication file
			assert(params >= 11);
		}
      }
    }
  }

// Print out all traffic communications with their volume info if traffic_in_bytes is true
if (GlobalParams::traffic_in_bytes) {
    cout << "*** Traffic Communication Table (with byte-converted volumes) ***" << endl;
    for (const auto& comm : traffic_communication_table) {
        // Format src vector as string
        string src_str;
        for (size_t i = 0; i < comm.src.size(); i++) {
            src_str += to_string(comm.src[i]);
            if (i < comm.src.size() - 1) src_str += " ";
        }
        
        // Format dst vector as string
        string dst_str;
        for (size_t i = 0; i < comm.dst.size(); i++) {
            dst_str += to_string(comm.dst[i]);
            if (i < comm.dst.size() - 1) dst_str += " ";
        }
        
        cout << "TaskID: " << comm.taskID << ", Src: [" << src_str << "], Dst: [" << dst_str 
            << "], Volumes (min/total): Src=" << comm.src_minVol << "/" << comm.src_totalVol 
            << ", Dst=" << comm.dst_minVol << "/" << comm.dst_totalVol << endl;
    }
    cout << "*** End of Traffic Communication Table ***" << endl;
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
    unsigned long long current_cycle = this->current_cycle;
    for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
        // To accommodate vector of src, use find() function
        bool found_src = false;
        bool found_dst = false;
        size_t src_pos = 0;
        size_t dst_pos = 0;
        int check_trn_state = TRN_WAIT;  // default state

        // store in tcomm variable for readability
        TrafficCommunication& tcomm = traffic_communication_table[i];

        // skip traffic row if traffic_used == true
        if (tcomm.traffic_used == true) {
            continue;
        }

        // ===== ELIGIBILITY TRACKING - START =====
        // Record wait start cycle for any task we encounter that doesn't have it set yet
        if (tcomm.wait_start_cycle == 0) {
            tcomm.wait_start_cycle = current_cycle;
        }
        
        // Check eligibility without changing function flow
        if (tcomm.wait_end_cycle == 0) {  // Only check if not already marked eligible
            bool eligible = true;
            
            // Check dependencies
            if (!(tcomm.waitID.size() == 1 && tcomm.waitID[0] == -1)) {
                for (int wait_id : tcomm.waitID) {
                    if (wait_id < 0) continue; // Skip no dependency marker
                    
                    // Find the dependency task
                    bool dependency_satisfied = false;
                    for (const auto& dep_comm : traffic_communication_table) {
                        if (dep_comm.taskID == wait_id) {
                            // Simplified dependency check without waitOP
                            if (dep_comm.traffic_used || dep_comm.compute_used || 
                                dep_comm.traffic_received) {
                                dependency_satisfied = true;
                            }
                            break;
                        }
                    }
                    
                    if (!dependency_satisfied) {
                        eligible = false;
                        break;
                    }
                }
            }
            
            // Mark as eligible if all dependencies satisfied
            if (eligible) {
                tcomm.wait_end_cycle = current_cycle;
                cout << "Task " << tcomm.taskID << " became eligible at cycle " << current_cycle 
                     << " (waited " << (current_cycle - tcomm.wait_start_cycle) << " cycles)" << endl;
            }
        }
        // ===== ELIGIBILITY TRACKING - END =====

        auto it = find(tcomm.src.begin(), tcomm.src.end(), src_id);
        found_src = (it != tcomm.src.end());
        
        if (!found_src) {
            continue;  // Skip if source not found
        }
        
        src_pos = distance(tcomm.src.begin(), it);
        
        // Handle Many-to-Many case
        if (tcomm.src.size() > 1 && tcomm.dst.size() > 1) {
            // Check if this source has any destinations in WAIT state
            bool has_wait_destinations = false;
            size_t wait_dst_pos = 0;
            
            for (size_t d = 0; d < tcomm.dst.size(); d++) {
                size_t idx = src_pos * tcomm.dst.size() + d;
                if (idx < tcomm.trn_complete.size() && tcomm.trn_complete[idx] == TRN_WAIT) {
                    has_wait_destinations = true;
                    wait_dst_pos = d;
                    break;
                }
            }
            
            if (has_wait_destinations) {
                // Found destination in WAIT state - mark as BUSY and return
                size_t idx = src_pos * tcomm.dst.size() + wait_dst_pos;
                cout << "DEBUG: (m2m) Traffic found for src_id = " << src_id 
                     << " to dst_id = " << tcomm.dst[wait_dst_pos] << " - return with TRN_WAIT" << endl;
                // tcomm.trn_complete[idx] = TRN_BUSY; // like PE handle TRN_BUSY tagging
                return tcomm;
            }
            
            // If no WAIT destinations but source is BUSY with any destination, return traffic
            for (size_t d = 0; d < tcomm.dst.size(); d++) {
                size_t idx = src_pos * tcomm.dst.size() + d;
                if (idx < tcomm.trn_complete.size() && tcomm.trn_complete[idx] == TRN_BUSY) {
                    cout << "DEBUG: (m2m) Source " << src_id << " already BUSY with destination "
                         << tcomm.dst[d] << " - returning traffic" << endl;
						// if (tcomm.dst[d] == 1) {
						// 	cout << "Press Enter to continue...";
						// 	cin.get();
						// }
                    return tcomm;
                }
            }
            
            // All destinations for this source are DONE - continue to next traffic
            continue;
        }
        // Handle Many-to-One case (existing logic)
        else if (tcomm.src.size() > 1 && tcomm.dst.size() == 1) {
            check_trn_state = tcomm.trn_complete[src_pos];

            if (check_trn_state == TRN_WAIT) {
                cout << "DEBUG: (m2o) Traffic Comm Table found for src_id = " << src_id 
                    << " return traffic" << endl;
                tcomm.trn_complete[src_pos] = TRN_BUSY;
                return tcomm;
            } 
            else if (check_trn_state == TRN_BUSY) {
                cout << "DEBUG: (m2o) Traffic Comm Table found for src_id = " << src_id 
                    << " trn_complete = TRN_BUSY, return traffic" << endl;
                return tcomm;
            }
            else if (check_trn_state == TRN_DONE) {
                cout << "DEBUG: (m2o) Traffic Comm Table found for src_id = " << src_id 
                    << " but trn_complete = TRN_DONE" << endl;
            }
        }
        // Handle One-to-Many or One-to-One case (existing logic)
        else if ((tcomm.src.size() == 1 && tcomm.dst.size() > 1) || 
                 (tcomm.src.size() == 1 && tcomm.dst.size() == 1)) {
            // Check if dst_id is being tagged or not in the trn_complete vector
            auto it = find(tcomm.trn_complete.begin(), tcomm.trn_complete.end(), TRN_WAIT);
            found_dst = (it != tcomm.trn_complete.end());
            dst_pos = distance(tcomm.trn_complete.begin(), it);

            if (found_dst && tcomm.trn_complete[dst_pos] == TRN_WAIT) {
                return tcomm;
            }
        }
    }
    
    return empty_comm;
}

void GlobalTrafficTable::moveReserveToTrafficCommunicationTable(const int src_id) {
/* // not used anymore
	vector <unsigned int> index_to_remove;

	for (unsigned int i = 0; i < reserved_traffic_communication_table.size(); i++) {
		TrafficCommunication reserved_comm = reserved_traffic_communication_table[i];
		auto src_it = find(reserved_comm.src.begin(), reserved_comm.src.end(), src_id);

		if (src_it != reserved_comm.src.end()){

			cout << "DEBUG: Found Reserved Traffic for src_id = " << src_id << endl;

			// loop and compare with traffic_communication_table
			// based on waitID vector in reserved_comm, find all matching taskID in traffic_communication_table
			// if waitOP state reserved_comm is satisfied, move to traffic_communication_table

			vector <bool> waitOP_state(reserved_comm.waitID.size(), false);

			for (unsigned int j = 0; j < traffic_communication_table.size(); j++) {
				TrafficCommunication tcomm = traffic_communication_table[j];
				
				// Check if all elements in trn_complete & cmp_complete match their respective completion states
				bool all_trn_done = true;
				bool all_cmp_done = true;
				

				for (size_t k = 0; k < tcomm.trn_complete.size(); k++) {
					if (tcomm.trn_complete[k] != TRN_DONE) all_trn_done = false;
					if (tcomm.cmp_complete[k] != CMP_DONE) all_cmp_done = false;
				}



				for (int l = 0; l < reserved_comm.waitID.size(); l++) {
					if (reserved_comm.waitID[l] == tcomm.taskID){
						if ((reserved_comm.waitOP == CMP && all_cmp_done) ||
                        (reserved_comm.waitOP == TRN && all_trn_done)){
							waitOP_state[l] = true;
						}
					}
				}

				// If all waitOP_state is true, meaining all taskID in waitID vector is satisfied
				// 	then move reserved_comm to traffic_communication_table
				if (all_of(waitOP_state.begin(), waitOP_state.end(), [](bool v) { return v; })) {
					traffic_communication_table.push_back(reserved_comm);
					index_to_remove.push_back(i);
					break;
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
*/
}

void GlobalTrafficTable::setTransmitComplete(const int task_ID, const int src_ID, const int dst_ID) {

    for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
        TrafficCommunication& comm = traffic_communication_table[i];

        if (comm.taskID == task_ID && comm.traffic_used == false) {
            // Many-to-Many case
            if (comm.src.size() > 1 && comm.dst.size() > 1) {
                auto src_it = find(comm.src.begin(), comm.src.end(), src_ID);
                auto dst_it = find(comm.dst.begin(), comm.dst.end(), dst_ID);
                
                if (src_it != comm.src.end() && dst_it != comm.dst.end()) {
                    size_t src_pos = distance(comm.src.begin(), src_it);
                    size_t dst_pos = distance(comm.dst.begin(), dst_it);
                    
                    // Calculate serialized index and mark as done
                    size_t idx = src_pos * comm.dst.size() + dst_pos;
                    if (idx < comm.trn_complete.size()) {
                        comm.trn_complete[idx] = TRN_DONE;
                        cout << "DEBUG: M2M transmission complete for task " << task_ID
                             << " from src=" << src_ID << " to dst=" << dst_ID << endl;
                    }
                }
            }
            // One-to-Many Case (existing logic)
            else if (comm.src.size() == 1 && comm.dst.size() > 1) {
                auto it = find(comm.dst.begin(), comm.dst.end(), dst_ID);
                if (it != comm.dst.end()) {
                    size_t pos = distance(comm.dst.begin(), it);
                    comm.trn_complete[pos] = TRN_DONE;
                }
            }
            // Many-to-One OR one-to-one case (existing logic)
            else if (comm.src.size() >= 1 && comm.dst.size() == 1) {
                auto it = find(comm.src.begin(), comm.src.end(), src_ID);
                if (it != comm.src.end()) {
                    size_t pos = distance(comm.src.begin(), it);
                    comm.trn_complete[pos] = TRN_DONE;
                }
            }

            // Check if all transmissions are complete
            bool all_done = true;
            for (const auto& trn_status : comm.trn_complete) {
                if (trn_status != TRN_DONE) {
                    all_done = false;
                    break;
                }
            }
            
            if (all_done && comm.traffic_used == false) {
                // ensure we only tag traffic_used once, for every taskID
                cout << "All traffic complete for taskID: " << task_ID 
                    << " at cycle " << current_cycle << endl;
                comm.traffic_used = true;
            }
            break;
        }
    }
}

void GlobalTrafficTable::setComputeComplete(const int task_ID, const int src_ID, const int local_ID) {
    unsigned long long current_cycle = this->current_cycle;
    for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
        TrafficCommunication& comm = traffic_communication_table[i];

        if (comm.taskID == task_ID && comm.compute_used == false) {

            // Many-to-Many case
            if (comm.src.size() > 1 && comm.dst.size() > 1) {
                auto src_it = find(comm.src.begin(), comm.src.end(), src_ID);
                auto dst_it = find(comm.dst.begin(), comm.dst.end(), local_ID);
                
                if (src_it != comm.src.end() && dst_it != comm.dst.end()) {
                    size_t src_pos = distance(comm.src.begin(), src_it);
                    size_t dst_pos = distance(comm.dst.begin(), dst_it);
                    
                    // Calculate serialized index and mark computation as done
                    size_t idx = src_pos * comm.dst.size() + dst_pos;
                    if (idx < comm.cmp_complete.size()) {
                        comm.cmp_complete[idx] = CMP_DONE;
                        cout << "DEBUG: M2M computation complete for task " << task_ID
                             << " from src=" << src_ID << " at dst=" << local_ID << endl;
                    }
                }
            }
            // One-to-Many case (existing logic)
            else if (comm.src.size() == 1 && comm.dst.size() > 1) {
                auto it = find(comm.dst.begin(), comm.dst.end(), local_ID);
                if (it != comm.dst.end()) {
                    size_t pos = distance(comm.dst.begin(), it);
                    comm.cmp_complete[pos] = CMP_DONE;
                }
            }
            // Many-to-One or One-to-One case (existing logic)
            else if (comm.src.size() >= 1 && comm.dst.size() == 1) {
                auto it = find(comm.src.begin(), comm.src.end(), src_ID);
                if (it != comm.src.end()) {
                    size_t pos = distance(comm.src.begin(), it);
                    comm.cmp_complete[pos] = CMP_DONE;
                }
            }
            
            // Check if all computation is complete
            bool all_done = true;
            for (const auto& cmp_status : comm.cmp_complete) {
                if (cmp_status != CMP_DONE) {
                    all_done = false;
                    break;
                }
            }
            
            if (all_done) {
                // Record compute end time
                comm.compute_end_cycle = current_cycle;
                cout << "DEBUG: All ComputeProcess() complete for taskID = " << task_ID 
                << " at cycle " << current_cycle
                << " (computation took " << (current_cycle - comm.compute_start_cycle) << " cycles)" << endl;
				comm.compute_used = true;
            }
            break;
        }
    }
}

void GlobalTrafficTable::setReceptionComplete(const int task_ID, const int src_ID, const int dst_ID) {
    unsigned long long current_cycle = this->current_cycle;
    for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
        TrafficCommunication& comm = traffic_communication_table[i];

        if (comm.taskID == task_ID) {

            if (comm.traffic_received == true) {
                cout << "DEBUG: Traffic Received has been done before, skip taskID: " << task_ID << endl;
                break;
            }

            // Many-to-Many case
            if (comm.src.size() > 1 && comm.dst.size() > 1) {
                auto src_it = find(comm.src.begin(), comm.src.end(), src_ID);
                auto dst_it = find(comm.dst.begin(), comm.dst.end(), dst_ID);
                
                if (src_it != comm.src.end() && dst_it != comm.dst.end()) {
                    size_t src_pos = distance(comm.src.begin(), src_it);
                    size_t dst_pos = distance(comm.dst.begin(), dst_it);
                    
                    // Calculate serialized index and mark as done
                    size_t idx = src_pos * comm.dst.size() + dst_pos;
                    if (idx < comm.rcv_complete.size()) {
                        comm.rcv_complete[idx] = RCV_DONE;
                        cout << "DEBUG: Reception complete for task " << task_ID
                             << " from src=" << src_ID << " to dst=" << dst_ID << endl;
                    }
                }
            }
            // One-to-Many Case
            else if (comm.src.size() == 1 && comm.dst.size() > 1) {
                auto it = find(comm.dst.begin(), comm.dst.end(), dst_ID);
                if (it != comm.dst.end()) {
                    size_t pos = distance(comm.dst.begin(), it);
                    comm.rcv_complete[pos] = RCV_DONE;
                }
            }
            // Many-to-One OR one-to-one case
            else if (comm.src.size() >= 1 && comm.dst.size() == 1) {
                auto it = find(comm.src.begin(), comm.src.end(), src_ID);
                if (it != comm.src.end()) {
                    size_t pos = distance(comm.src.begin(), it);
                    comm.rcv_complete[pos] = RCV_DONE;
                }
            }

            if (comm.is_self_compute) {
                // set all reception to done
                comm.rcv_complete[0] = RCV_DONE;
                cout << "DEBUG: Self compute reception complete for task " << task_ID
                     << " from src=" << src_ID << " to dst=" << dst_ID << endl;
            }

            // Check if all receptions are complete
            bool all_rcv_done = true;
            for (const auto& rcv_status : comm.rcv_complete) {
                if (rcv_status != RCV_DONE) {
                    all_rcv_done = false;
                    break;
                }
            }
            
            if (all_rcv_done && !comm.traffic_received) {
                // Record reception end time
                comm.receive_end_cycle = current_cycle;
                
                // Mark all timing data as valid
                if (comm.wait_start_cycle > 0 && comm.wait_end_cycle > 0 &&
                    comm.transmit_start_cycle > 0 && comm.transmit_end_cycle > 0) {
                    comm.timing_valid = true;
                }
                
                cout << "All reception complete for taskID: " << task_ID 
                     << " at cycle " << current_cycle
                     << " (reception took " << (current_cycle - comm.receive_start_cycle) << " cycles)" << endl;
                comm.traffic_received = true;  // Set flag indicating all receptions are complete
            }
            break;
        }
    }
}

bool GlobalTrafficTable::checkReceptionDependencies(const vector<int>& waitIDs) {
    for (int waitID : waitIDs) {
        if (waitID == -1) continue;  // No dependency
        
        // Check if the dependency task has its reception completed
        bool found_completed = false;
        for (auto& comm : traffic_communication_table) {
            if (comm.taskID == waitID && comm.traffic_received) {
                found_completed = true;
                break;
            }
        }
        
        if (!found_completed) return false;  // At least one dependency not satisfied
    }
    return true;  // All dependencies satisfied
}

void GlobalTrafficTable::updateReceivedTraffic(const int task_ID, const int src_ID, const int dst_ID) {
    // Update the traffic communication table to mark the reception as complete
    // TODO: only works for one to one traffic for now, need to use ternary operator like compute task
    for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
		TrafficCommunication& comm = traffic_communication_table[i];
		if (comm.taskID == task_ID) {
			comm.received_traffic ++;
            cout << "DEBUG: received_traffic for task " << task_ID
                     << " [" << comm.received_traffic << "|" << comm.dst_totalVol <<"]" << endl;

            if (comm.received_traffic == comm.dst_totalVol) {
                cout << "DEBUG: Done all received_traffic for task " << task_ID
                     << " [" << comm.received_traffic << "|" << comm.dst_totalVol <<"]" << endl;
                setReceptionComplete(task_ID, src_ID, dst_ID);
            }
		}
	}
}

TrafficCommunication GlobalTrafficTable::getsrcID(const int task_ID) {

	for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
		TrafficCommunication comm = traffic_communication_table[i];
		if (comm.taskID == task_ID) {
			return comm;
		}
	}
	// HG: return empty TrafficCommunication, if not matching task_ID found
	assert("Error in Traffic Table, no such taskID!");
	return empty_comm;
}

TrafficCommunication GlobalTrafficTable::getEmptyComm() {
	return empty_comm;
}

// Convert bytes to data_volume units based on flit size
int GlobalTrafficTable::bytesToDataVolume(int bytes) {
    // If traffic_in_bytes flag is false, return the original value (backward compatibility)
    if (!GlobalParams::traffic_in_bytes)
        return bytes;

    // Special cases: if bytes is -1 or 0, return as-is
    if (bytes <= 0)
        return bytes;
        
    // Calculate flit size in bytes (from bits)
    int flit_size_bytes = GlobalParams::flit_size / 8;
    
    // Round up to ensure all data is transmitted (ceiling division)
    // This converts bytes to equivalent data_volume units
    return (bytes + flit_size_bytes - 1) / flit_size_bytes;
}

int GlobalTrafficTable::occurrencesAsSource(const int src_id)
{
  int count = 0;

  for (unsigned int i = 0; i < traffic_table.size(); i++)
    if (traffic_table[i].src == src_id)
      count++;

  return count;
}

void GlobalTrafficTable::markTransmitStart(const int task_ID, const int src_ID) {
    for (auto& comm : traffic_communication_table) {
        if (comm.taskID == task_ID && comm.transmit_start_cycle == 0) {  // Prevent double-marking
            comm.transmit_start_cycle = current_cycle;
            cout << "Task " << task_ID << " started transmission at cycle " 
                 << current_cycle << " from PE " << src_ID << endl;
            break;
        }
    }
}

void GlobalTrafficTable::markTransmitEnd(const int task_ID, const int src_ID, const int dst_ID) {
    for (auto& comm : traffic_communication_table) {
        if (comm.taskID == task_ID ) {
            comm.transmit_end_cycle = current_cycle;
            cout << "Task " << task_ID << " completed transmission at cycle " 
                 << current_cycle << " (took " << (current_cycle - comm.transmit_start_cycle) 
                 << " cycles) from PE " << src_ID << " to PE " << dst_ID << endl;
            break;
        }
    }
}
void GlobalTrafficTable::markComputeStart(const int task_ID, const int dst_ID) {
    for (auto& comm : traffic_communication_table) {
        if (comm.taskID == task_ID && comm.compute_start_cycle == 0) {  // Prevent double-marking
            comm.compute_start_cycle = current_cycle;
            cout << "Task " << task_ID << " started computation at cycle " 
                 << current_cycle << " at PE " << dst_ID << endl;
            break;
        }
    }
}

void GlobalTrafficTable::markReceiveStart(const int task_ID, const int src_ID, const int dst_ID) {
    for (auto& comm : traffic_communication_table) {
        if (comm.taskID == task_ID && comm.receive_start_cycle == 0) {  // Prevent double-marking
            comm.receive_start_cycle = current_cycle;
            cout << "Task " << task_ID << " started reception at cycle " 
                 << current_cycle << " at PE " << dst_ID << endl;
            break;
        }
    }
}
