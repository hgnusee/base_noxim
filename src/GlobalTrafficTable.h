/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the definition of the global traffic table
 */

#ifndef __NOXIMGLOBALTRAFFIC_TABLE_H__
#define __NOXIMGLOBALTRAFFIC_TABLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm> // HG: to use std::find
#include "DataStructs.h"
#include "Utils.h"
#include <unordered_map>



using namespace std;

// Structure used to store information into the table
struct Communication {
  int src;			// ID of the source node (PE)
  int dst;			// ID of the destination node (PE)
  double pir;			// Packet Injection Rate for the link
  double por;			// Probability Of Retransmission for the link
  int t_on;			// Time (in cycles) at which activity begins
  int t_off;			// Time (in cycles) at which activity ends
  int t_period;		        // Period after which activity starts again
};

// HG: Structure to store traffic communication
// taskID, src, dst, data_volume, waitID, waitOP, traffic_used, trn_complete, cmp_complete
struct TrafficCommunication {
  int taskID;
  int layerID;
  vector < int > src; // replace src with vector of src
  vector < int > dst; // replace dst with vector of dst
  int src_minVol;
  int src_totalVol;
  int dst_minVol;
  int dst_totalVol;
  vector < int >  waitID; // replace waitID with vector of waitID
  vector < int >  nextID;
  int waitOP;
  bool traffic_used;
  bool compute_used;
  // trn/cmp_complete flag uses int to act as state for each src id, for multiple src to control. 
  //    the values should not be less than 0
  vector < int > trn_complete; // flag for transmit by src PE is done
  vector < int > cmp_complete; // flag for computation complete in dst PE

  // multicast support
  int traffic_type;  // Type of traffic: unicast, multicast, broadcast

  // fields for reception tracking
  vector<int> rcv_complete;  // RCV_WAIT, RCV_BUSY, RCV_DONE
  bool traffic_received;     // Flag indicating all destinations have received data
  
  // flags for traffic types
  bool is_self_compute;      // Flag for self-compute tasks (src==dst)
  bool is_wait_reception;    // Flag for traffic waiting on reception completion

  // recevied traffic (in flits, at dst) counter for taskID
  int received_traffic;

  // Timing tracking fields - all in simulation cycles
  unsigned long long wait_start_cycle;    // When dependencies began being checked
  unsigned long long wait_end_cycle;      // When all dependencies were satisfied
  unsigned long long transmit_start_cycle; // When first transmission started
  unsigned long long transmit_end_cycle;   // When all transmissions completed
  unsigned long long compute_start_cycle;  // When computation started
  unsigned long long compute_end_cycle;    // When computation completed
  unsigned long long receive_start_cycle;  // When reception started
  unsigned long long receive_end_cycle;    // When all receptions completed
  bool timing_valid;                      // Whether timing data is complete

  TrafficCommunication() {
    traffic_type = T_UNICAST;
    received_traffic = 0;
    // timing initializations
    wait_start_cycle = wait_end_cycle = 0;
    transmit_start_cycle = transmit_end_cycle = 0;
    compute_start_cycle = compute_end_cycle = 0;
    receive_start_cycle = receive_end_cycle = 0;
    timing_valid = false;
  }

  // HG: Below are all heper functions for setting up index based src-dst pair status tracking
  // ###### Sat Mar 29 14:24:37 SGT 2025
  // Helper function to calculate index from src_pos and dst_pos
  size_t getIndex(size_t src_pos, size_t dst_pos) const {
      return src_pos * dst.size() + dst_pos;
  }
  
  // Helper function to get transaction state for specific src-dst pair
  int getTrnState(size_t src_pos, size_t dst_pos) const {
      return trn_complete[getIndex(src_pos, dst_pos)];
  }
  
  // Helper function to set transaction state for specific src-dst pair
  void setTrnState(size_t src_pos, size_t dst_pos, int state) {
      trn_complete[getIndex(src_pos, dst_pos)] = state;
  }
  
  // Similar helpers for compute state
  int getCmpState(size_t src_pos, size_t dst_pos) const {
      return cmp_complete[getIndex(src_pos, dst_pos)];
  }
  
  void setCmpState(size_t src_pos, size_t dst_pos, int state) {
      cmp_complete[getIndex(src_pos, dst_pos)] = state;
  }

};

class GlobalTrafficTable {

  public:

    GlobalTrafficTable();

    // Load traffic table from file. Returns true if ok, false otherwise
    bool load(const char *fname);

    // HG: Load TrafficCommunication table from file
    // similar to bool load()
    bool loadTrafficFile(const char *fname);

    // Returns the cumulative pir por along with a vector of pairs. The
    // first component of the pair is the destination. The second
    // component is the cumulative shotting probability.
    double getCumulativePirPor(const int src_id, const int ccycle, const bool pir_not_por, vector < pair < int, double > > &dst_prob);
    
    // HG: get parsed Traffic Communication Table
    // similar to getCumulativePirPor()
    // uses exact transaction info found in Traffic Communication Table
    TrafficCommunication& getTrafficCommunicationTable(const int src_id);

    // HG: Move reserved transaction from Traffic Communication Table to Traffic Tabl
    void moveReserveToTrafficCommunicationTable(const int src_id);
    
    // HG: set trn_complete flag in Traffic Communication Table
    void setTransmitComplete(const int task_ID, const int src_ID, const int dst_ID);

    // HG: set cmp_complete flag in Traffic Communication Table
    void setComputeComplete(const int task_ID, const int dst_ID, const int local_ID);

    // HG: set rcv_complete flag in Traffic Communication Table
    void setReceptionComplete(const int task_ID, const int src_ID, const int dst_ID);

    // start markers for traffic timing
    void markTransmitStart(const int task_ID, const int src_ID);
    void markTransmitEnd(const int task_ID, const int src_ID, const int dst_ID);
    void markComputeStart(const int task_ID, const int dst_ID);
    void markReceiveStart(const int task_ID, const int src_ID, const int dst_ID);

    // HG: Method to check if dependencies reception is complete
    bool checkReceptionDependencies(const vector<int>& waitIDs);

    // HG: update received_traffic counter in Traffic Communication Table
    void updateReceivedTraffic(const int task_ID, const int src_ID, const int dst_ID);

    // HG: get vector of src_id from Traffic Communication Table
    TrafficCommunication getsrcID(const int task_ID);

    // HG: Convert bytes to data volume (in flits)
    int bytesToDataVolume(int bytes);
    
    // HG: get empty comm from other functions
    TrafficCommunication getEmptyComm();

    // Returns the number of occurrences of soruce src_id in the traffic
    // table
    int occurrencesAsSource(const int src_id);

    // Getter for the traffic communication table
    const vector<TrafficCommunication>& getTCommunicationTable() const {
      return traffic_communication_table;
      
    }

    // Helper function to get simulation time from PE
    void updateCurrentCycle(unsigned long long cycle) {
        current_cycle = cycle;
    }
    
    unsigned long long getCurrentCycle() const {
        return current_cycle;
    }



  private:

     vector < Communication > traffic_table;
     // HG: Create a vector for TrafficCommunication
     vector < TrafficCommunication > traffic_communication_table;
     //  HG: reserved_traffic_communication_table for holding next PE or waiting PE
     // Not used anymore
    //  vector < TrafficCommunication > reserved_traffic_communication_table;

    // get simulation time
    unsigned long long current_cycle;

      // HG: 'empty' transaction to be returned in no entry found in traffic comm table
      TrafficCommunication empty_comm;

    unordered_map<int, size_t> task_id_to_index;  // Maps task IDs to their indices in the vector




};

#endif
