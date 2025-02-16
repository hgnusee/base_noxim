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
// src dst data_volume waitPE nextPE
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
	int src, dst;	// Mandatory
	int data_vol, waitPE, nextPE;

	int params =
	  sscanf(line, "%d %d %d %d %d", &src, &dst, &data_vol,
		 &waitPE, &nextPE);
	if (params >= 2 && params <= 5) {
	  // Create a communication from the parameters read on the line
	  TrafficCommunication TrafficCommunication;

	  // Mandatory fields
	  TrafficCommunication.src = src;
	  TrafficCommunication.dst = dst;
	  TrafficCommunication.data_volume = data_vol;
	  TrafficCommunication.waitPE = waitPE;
	  TrafficCommunication.nextPE = nextPE;
	  TrafficCommunication.used_traffic = false; // HG: All traffic are unused initially

	  // Bucket the Traffic into reserved_traffic_communication_table or not
	  if (waitPE == -1 && nextPE == -1) {
	    assert("Wrong Traffic! Both waitPE and nextPE are -1");
	  } else if (waitPE == -1 && nextPE != -1) {
	    // Add to vector of communications
	    traffic_communication_table.push_back(TrafficCommunication);
	  } else if (waitPE != -1 && nextPE == -1) {
	    // Add to the reserved vector of communications
	    reserved_traffic_communication_table.push_back(TrafficCommunication);
	  } else if (waitPE != -1 && nextPE != -1) {
	    // Add to the reserved vector of communications
	    reserved_traffic_communication_table.push_back(TrafficCommunication);
	  } else {
	    assert("Wrong Traffic! Ennsure waitPE and nextPE are set!");
	  }
	} else {
		// ensure all params must be present in traffic communication file
		assert(params == 5);
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

TrafficCommunication GlobalTrafficTable::getTrafficCommunicationTable(const int src_id)
{
  TrafficCommunication comm;

  // intialize TrafficCommunication struct (to act as a empty struct)
  comm = { 0, 0, 0, 0, 0, true};

  for (unsigned int i = 0; i < traffic_communication_table.size(); i++) {
	TrafficCommunication comm = traffic_communication_table[i];
	if (comm.src == src_id && comm.used_traffic == false) { // HG: Check if traffic is used
		// remove transaction from transaction communication table once used
		// traffic_communication_table.erase(traffic_communication_table.begin() + i);
		cout << "DEBUG: Traffic Communication Table found for src_id = " << src_id << endl;
		// return transaction to Processing Element to make packet
	  return comm;
	}
  }

  // HG:return empty TrafficCommunication, if not matching src_id found
//   cout << "DEBUG: No Traffic Communication Table found for src_id = " << src_id << endl;
  return comm;
}

void GlobalTrafficTable::moveReserveToTrafficCommunicationTable(const int nextPE, const int src_id) {
	
	cout << "DEBUG: Print Reserve Table before MOVING" << endl;
	// Print reserved_traffic_communication_table
	for (const auto& comm : reserved_traffic_communication_table) {
		cout << "src: " << comm.src << ", dst: " << comm.dst << ", data_volume: " << comm.data_volume
				<< ", waitPE: " << comm.waitPE << ", nextPE: " << comm.nextPE << endl;
	}
	TrafficCommunication comm;
	// intialize TrafficCommunication struct (to act as a empty struct)
	comm = { 0, 0, 0, 0, 0 };
	
	cout << "Target src_id: " << src_id << ", nextPE: " << nextPE << endl;

	// move reserved transaction from Traffic Communication Table to Traffic Tabl
	for (unsigned int i = 0; i < reserved_traffic_communication_table.size(); i++) {
		// check if the reserved table contains: 
		// comm.src is based on the nextPE label of the earlier transaction
		// comm.waitPE in the reserve table that matches src_id of the earlier transaction,
		//     since this current transaction is waiting for earlier transaction to complete
		TrafficCommunication comm = reserved_traffic_communication_table[i];
		if (comm.src == nextPE && comm.waitPE == src_id) {
			// remove transaction from reserved transaction communication table
			reserved_traffic_communication_table.erase(reserved_traffic_communication_table.begin() + i);
			// add transaction to transaction communication table
			traffic_communication_table.push_back(comm);
			cout << "DEBUG: Found Reserved Traffic for src_id = " << src_id << " and nextPE = " << nextPE << endl;
			return;
		}
	}
	cout << "DEBUG: No Reserved Traffic Communication Table found for dst_id = " << src_id << " and nextPE = " << nextPE << endl;

}

// for DEBUG use when building moveReserveToTrafficCommunicationTable
// repurpose to get reserve traffic communication table
TrafficCommunication GlobalTrafficTable::getReserveTrafficCommunicationTable(const int src_id) {
	
	cout<<"DEBUG: size of reserved_traffic: "<< reserved_traffic_communication_table.size() << endl;
	TrafficCommunication comm;
	// intialize TrafficCommunication struct (to act as a empty struct)
	comm = { 0, 0, 0, 0, 0, true };
	
	for (unsigned int i = 0; i < reserved_traffic_communication_table.size(); i++) {
		TrafficCommunication comm = reserved_traffic_communication_table[i];
		// get reserve traffic table where local_id == src_id (of reserved traffic)
			// based on this, check used_traffic == true, nextPE == local_id
				// return shot = true;
			// else return empty comm;
		
		if (comm.src == src_id && comm.used_traffic == false && comm.waitPE >= 0) {
			
			for (unsigned int j = 0; j < traffic_communication_table.size(); j++) {
				TrafficCommunication comm_tmp = traffic_communication_table[j];
				if (comm_tmp.nextPE == src_id && comm_tmp.used_traffic == true && comm_tmp.src == comm.waitPE) {
					// remove transaction from transaction communication table
					// traffic_communication_table.erase(traffic_communication_table.begin() + j);
					// add transaction to reserved transaction communication table
					comm.used_traffic = true; // set as used traffic
					traffic_communication_table.push_back(comm); // move to traffic comm table
					cout << "DEBUG: Found Reserved + Matching Used Traffic for src_id = " << src_id << endl;
					return comm;
				}
			}
			
		}
	}
	
	// HG:return empty TrafficCommunication, if not matching src_id found
	cout << "DEBUG: No Reserved + Matching Used Traffic found for src_id = " << src_id << endl;
	
	return comm;
}

int GlobalTrafficTable::occurrencesAsSource(const int src_id)
{
  int count = 0;

  for (unsigned int i = 0; i < traffic_table.size(); i++)
    if (traffic_table[i].src == src_id)
      count++;

  return count;
}
