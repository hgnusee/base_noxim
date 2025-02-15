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
#include "DataStructs.h"

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
// 
struct TrafficCommunication {
  int src;
  int dst;
  int data_volume;
  int waitPE;
  int nextPE;
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
    TrafficCommunication getTrafficCommunicationTable(const int src_id);

    // HG: Move reserved transaction from Traffic Communication Table to Traffic Tabl
    void moveReserveToTrafficCommunicationTable(const int nextPE, const int src_id);
    
    // HG: get reserve table traffic that matches nextPE
    TrafficCommunication getReserveTrafficCommunicationTable(const int nextPE, const int src_id);
    

    // Returns the number of occurrences of soruce src_id in the traffic
    // table
    int occurrencesAsSource(const int src_id);

  private:

     vector < Communication > traffic_table;
     // HG: Create a vector for TrafficCommunication
     vector < TrafficCommunication > traffic_communication_table;
     //  HG: reserved_traffic_communication_table for holding next PE or waiting PE
     vector < TrafficCommunication > reserved_traffic_communication_table;

};

#endif
