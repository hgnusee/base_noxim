/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the global statistics
 */

#ifndef __NOXIMGLOBALSTATS_H__
#define __NOXIMGLOBALSTATS_H__

#include <iostream>
#include <vector>
#include <iomanip>
#include "NoC.h"
#include "Tile.h"
#include "GlobalTrafficTable.h"
using namespace std;
// Add global stall statistics structure
struct GlobalStallMatrices {
  vector<vector<unsigned long>> pe_to_router_stalls;
  vector<vector<unsigned long>> router_to_router_stalls;  
  vector<vector<unsigned long>> total_stalls;
};

struct GlobalStallStats {
  // Matrices for heatmaps
  vector<vector<unsigned long>> pe_to_router_stalls;
  vector<vector<unsigned long>> router_to_router_stalls;
  vector<vector<unsigned long>> total_stalls;
  
  // Directional aggregates
  unsigned long pe_to_router_stalls_by_direction[DIRECTIONS + 2];
  unsigned long router_to_router_stalls_by_direction[DIRECTIONS + 2];
  
  // Stall reasons
  unsigned long reservation_stalls;
  unsigned long buffer_full_stalls;
  unsigned long vc_busy_stalls;
  unsigned long already_reserved_stalls;
};

struct TaskTimeline {
  int task_id;
  int layer_id;
  unsigned long long wait_start;
  unsigned long long wait_end;
  unsigned long long transmit_start;
  unsigned long long transmit_end;
  unsigned long long compute_start;
  unsigned long long compute_end;
  unsigned long long receive_start;
  unsigned long long receive_end;
  
  // Convenience method to get total time
  unsigned long long getTotalTime() const {
      return receive_end > 0 ? receive_end - wait_start : 0;
  }
  
  // Convenience methods to get phase durations
  unsigned long long getWaitTime() const { return wait_end > 0 ? wait_end - wait_start : 0; }
  unsigned long long getTransmitTime() const { 
      return transmit_end > 0 ? transmit_end - transmit_start : 0; 
  }
  unsigned long long getComputeTime() const { 
      return compute_end > 0 ? compute_end - compute_start : 0; 
  }
  unsigned long long getReceiveTime() const { 
      return receive_end > 0 ? receive_end - receive_start : 0; 
  }
};

class GlobalStats {

  public:

    GlobalStats(const NoC * _noc, const GlobalTrafficTable* _traffic_communication_table);

    // Returns the aggregated average delay (cycles)
    double getAverageDelay();

    // Returns the aggragated average delay (cycles) for communication src_id->dst_id
    double getAverageDelay(const int src_id, const int dst_id);

    // Returns the max delay
    double getMaxDelay();

    // Returns the max delay (cycles) experimented by destination
    // node_id. Returns -1 if node_id is not destination of any
    // communication
    double getMaxDelay(const int node_id);

    // Returns the max delay (cycles) for communication src_id->dst_id
    double getMaxDelay(const int src_id, const int dst_id);

    // Returns tha matrix of max delay for any node of the network
     vector < vector < double > > getMaxDelayMtx();

    // Returns the aggregated average throughput (flits/cycles)
    double getAggregatedThroughput();

    // Returns the average throughput per IP (flit/cycles/IP)
    double getThroughput();

    // Returns the average throughput considering only a active IP (flit/cycles/IP)
    double getActiveThroughput();

    // Returns the aggregated average throughput (flits/cycles) for
    // communication src_id->dst_id
    double getAverageThroughput(const int src_id, const int dst_id);

    // Returns the total number of received packets
    unsigned int getReceivedPackets();

    // Returns the total number of received flits
    unsigned int getReceivedFlits();

    // number of packets that used the wireless network
    unsigned int getWirelessPackets();


    // Returns the number of routed flits for each router
     vector < vector < unsigned long > > getRoutedFlitsMtx();

    // Returns the total dyamic power
    double getDynamicPower();
    // Returns the total static power
    double getStaticPower();

    // Returns the total power
    double getTotalPower() { return getDynamicPower()+getStaticPower(); }

    // Shows global statistics
    void showStats(std::ostream & out = std::cout, bool detailed = false);

    void showBufferStats(std::ostream & out);

    void showPowerBreakDown(std::ostream & out);

    void showPowerManagerStats(std::ostream & out);

    double getReceivedIdealFlitRatio();

    // Returns network throughput in gigabits per second
    double getNetworkThroughputGbps();

    // Returns network throughput in gigabytes per second
    double getNetworkThroughputGBps();
    
    // Returns average IP throughput in gigabits per second
    double getIPThroughputGbps();
    
    // Returns average IP throughput in gigabytes per second
    double getIPThroughputGBps();

    GlobalStallMatrices stall_matrices;
    GlobalStallStats stall_stats;

    //  add global stall stats
    void collectStallStats();
    // global stall stats visualization
    void showStallStats(std::ostream & out);
    void generateStallHeatmap(std::ostream & out);
    void showStallsByReasonPerNode(std::ostream & out);
    void showTrafficCompletionStats(std::ostream & out);
    void printTrafficCompletionStats();

    // traffic timing stats
    void showTrafficTimingStats(std::ostream & out = std::cout);
    void exportTaskTimingData(const string& filename);
    void generateHistogramData();
    void outputHistogramData(const string& filename, const vector<unsigned long long>& data);

    // New methods for "true" throughput metrics
    unsigned long long getActualSimulationEndTime();
    double getTrueAggregatedThroughput();
    double getTrueThroughput();
    double getTrueNetworkThroughputGbps();
    double getTrueNetworkThroughputGBps();
    double getTrueIPThroughputGbps();
    double getTrueIPThroughputGBps();



#ifdef TESTING
    unsigned int drained_total;
#endif

  private:
    const NoC *noc;
    // HG: Reference to Traffic Communication Table
    const GlobalTrafficTable* traffic_communication_table;
    void updatePowerBreakDown(map<string,double> &dst,PowerBreakdown* src);
};

#endif
