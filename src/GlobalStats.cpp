/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementaton of the global statistics
 */

#include "GlobalStats.h"
using namespace std;

GlobalStats::GlobalStats(const NoC * _noc, const GlobalTrafficTable* _traffic_communication_table)
{
    noc = _noc;
    traffic_communication_table = _traffic_communication_table;

	#ifdef TESTING
    drained_total = 0;
	#endif
}

double GlobalStats::getAverageDelay()
{
    unsigned int total_packets = 0;
    double avg_delay = 0.0;

    if (GlobalParams::topology == TOPOLOGY_MESH)
    {
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++) 
	    {
		unsigned int received_packets =
		    noc->t[x][y]->r->stats.getReceivedPackets();

		if (received_packets) 
		{
		    avg_delay +=
			received_packets *
			noc->t[x][y]->r->stats.getAverageDelay();
		    total_packets += received_packets;
		}
	    }
    }
    else // other delta topologies
    { 
	for (int y = 0; y < GlobalParams::n_delta_tiles; y++)
	{
	    unsigned int received_packets =
		noc->core[y]->r->stats.getReceivedPackets();

	    if (received_packets) 
	    {
		avg_delay +=
		    received_packets *
		    noc->core[y]->r->stats.getAverageDelay();
		total_packets += received_packets;
	    }
	}

    }


    avg_delay /= (double) total_packets;

    return avg_delay;
}



double GlobalStats::getAverageDelay(const int src_id,
					 const int dst_id)
{
    Tile *tile = noc->searchNode(dst_id);

    assert(tile != NULL);

    return tile->r->stats.getAverageDelay(src_id);
}

double GlobalStats::getMaxDelay()
{
    double maxd = -1.0;

    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++) 
	    {
		Coord coord;
		coord.x = x;
		coord.y = y;
		int node_id = coord2Id(coord);
		double d = getMaxDelay(node_id);
		if (d > maxd)
		    maxd = d;
	    }

    }
    else  // other delta topologies 
    {
	for (int y = 0; y < GlobalParams::n_delta_tiles; y++)
	{
	    double d = getMaxDelay(y);
	    if (d > maxd)
		maxd = d;
	}
    }

    return maxd;
}

double GlobalStats::getMaxDelay(const int node_id)
{
    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
	Coord coord = id2Coord(node_id);

	unsigned int received_packets =
	    noc->t[coord.x][coord.y]->r->stats.getReceivedPackets();

	if (received_packets)
	    return noc->t[coord.x][coord.y]->r->stats.getMaxDelay();
	else
	    return -1.0;
    }
    else // other delta topologies
    {
	unsigned int received_packets =
	    noc->core[node_id]->r->stats.getReceivedPackets();
	if (received_packets)
	    return noc->core[node_id]->r->stats.getMaxDelay();
	else
	    return -1.0;
    }

}

double GlobalStats::getMaxDelay(const int src_id, const int dst_id)
{
    Tile *tile = noc->searchNode(dst_id);

    assert(tile != NULL);

    return tile->r->stats.getMaxDelay(src_id);
}

vector < vector < double > > GlobalStats::getMaxDelayMtx()
{
    vector < vector < double > > mtx;

    assert(GlobalParams::topology == TOPOLOGY_MESH); 

    mtx.resize(GlobalParams::mesh_dim_y);
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	mtx[y].resize(GlobalParams::mesh_dim_x);

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) 
	{
	    Coord coord;
	    coord.x = x;
	    coord.y = y;
	    int id = coord2Id(coord);
	    mtx[y][x] = getMaxDelay(id);
	}

    return mtx;
}

double GlobalStats::getAverageThroughput(const int src_id, const int dst_id)
{
    Tile *tile = noc->searchNode(dst_id);

    assert(tile != NULL);

    return tile->r->stats.getAverageThroughput(src_id);
}

/*
double GlobalStats::getAverageThroughput()
{
    unsigned int total_comms = 0;
    double avg_throughput = 0.0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    unsigned int ncomms =
		noc->t[x][y]->r->stats.getTotalCommunications();

	    if (ncomms) {
		avg_throughput +=
		    ncomms * noc->t[x][y]->r->stats.getAverageThroughput();
		total_comms += ncomms;
	    }
	}

    avg_throughput /= (double) total_comms;

    return avg_throughput;
}
*/

double GlobalStats::getAggregatedThroughput()
{
    int total_cycles = GlobalParams::simulation_time - GlobalParams::stats_warm_up_time;

    return (double)getReceivedFlits()/(double)(total_cycles);
}

unsigned int GlobalStats::getReceivedPackets()
{
    unsigned int n = 0;

    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
    	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
		for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    n += noc->t[x][y]->r->stats.getReceivedPackets();
    }
    else // other delta topologies
    {
    	for (int y = 0; y < GlobalParams::n_delta_tiles; y++)
	    n += noc->core[y]->r->stats.getReceivedPackets();
    }

    return n;
}

unsigned int GlobalStats::getReceivedFlits()
{
    unsigned int n = 0;
    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
		n += noc->t[x][y]->r->stats.getReceivedFlits();
#ifdef TESTING
		drained_total += noc->t[x][y]->r->local_drained;
#endif
	    }
    }
    else // other delta topologies
    {
	for (int y = 0; y < GlobalParams::n_delta_tiles; y++)
	{
	    n += noc->core[y]->r->stats.getReceivedFlits();
#ifdef TESTING
	    drained_total += noc->core[y]->r->local_drained;
#endif
	}
    }

    return n;
}

double GlobalStats::getThroughput()
{
    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
	int number_of_ip = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
	return (double)getAggregatedThroughput()/(double)(number_of_ip);
    }
    else // other delta topologies
    {
	int number_of_ip = GlobalParams::n_delta_tiles;
	return (double)getAggregatedThroughput()/(double)(number_of_ip);
    }
}

// Only accounting IP that received at least one flit
double GlobalStats::getActiveThroughput()
{
    int total_cycles =
	GlobalParams::simulation_time -
	GlobalParams::stats_warm_up_time;
    unsigned int n = 0;
    unsigned int trf = 0;
    unsigned int rf ;
    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++) 
	    {
		rf = noc->t[x][y]->r->stats.getReceivedFlits();

		if (rf != 0)
		    n++;

		trf += rf;
	    }
    }
    else // other delta topologies
    {
	for (int y = 0; y < GlobalParams::n_delta_tiles; y++)
	{
	    rf = noc->core[y]->r->stats.getReceivedFlits();

	    if (rf != 0)
		n++;

	    trf += rf;
	}
    }

    return (double) trf / (double) (total_cycles * n);

}

vector < vector < unsigned long > > GlobalStats::getRoutedFlitsMtx()
{

    vector < vector < unsigned long > > mtx;
    assert (GlobalParams::topology == TOPOLOGY_MESH); 

    mtx.resize(GlobalParams::mesh_dim_y);
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	mtx[y].resize(GlobalParams::mesh_dim_x);

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    mtx[y][x] = noc->t[x][y]->r->getRoutedFlits();


    return mtx;
}

unsigned int GlobalStats::getWirelessPackets()
{
    unsigned int packets = 0;

    // Wireless noc
    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
            it != GlobalParams::hub_configuration.end();
            ++it)
    {
	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	packets+= h->wireless_communications_counter;
    }
    return packets;
}

double GlobalStats::getDynamicPower()
{
    double power = 0.0;

    // Electric noc
    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
		power += noc->t[x][y]->r->power.getDynamicPower();
    }
    else // other delta topologies
    {
	int stg = log2(GlobalParams::n_delta_tiles);
	int sw = GlobalParams::n_delta_tiles/2; //sw: switch number in each stage
	// Dimensions of the delta switch block network
	int dimX = stg;
	int dimY = sw;

	// power for delta topologies cores
	for (int y = 0; y < GlobalParams::n_delta_tiles; y++)
	    power += noc->core[y]->r->power.getDynamicPower();

	// power for delta topologies switches 
	for (int y = 0; y < dimY; y++)
	    for (int x = 0; x < dimX; x++)
		power += noc->t[x][y]->r->power.getDynamicPower();
    }

    // Wireless noc
    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
	    it != GlobalParams::hub_configuration.end();
	    ++it)
    {
	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	power+= h->power.getDynamicPower();
    }
    return power;
}

double GlobalStats::getStaticPower()
{
    double power = 0.0;

    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
    	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
		for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    power += noc->t[x][y]->r->power.getStaticPower();
    }
    else // other delta topologies
    {
	int stg = log2(GlobalParams::n_delta_tiles);
	int sw = GlobalParams::n_delta_tiles/2; //sw: switch number in each stage
	// Dimensions of the delta switch block network
	int dimX = stg;
	int dimY = sw;
	// power for delta topologies switches 
	for (int y = 0; y < dimY; y++)
	    for (int x = 0; x < dimX; x++)
		power += noc->t[x][y]->r->power.getDynamicPower();

	// delta cores
    	for (int y = 0; y < GlobalParams::n_delta_tiles; y++)
	    power += noc->core[y]->r->power.getStaticPower();
    }

    // Wireless noc
    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
            it != GlobalParams::hub_configuration.end();
            ++it)
    {
	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	power+= h->power.getStaticPower();
    }
    return power;
}

void GlobalStats::showStats(std::ostream & out, bool detailed)
{
    if (detailed) 
    {
	assert (GlobalParams::topology == TOPOLOGY_MESH); 
	out << endl << "detailed = [" << endl;

	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
		noc->t[x][y]->r->stats.showStats(y * GlobalParams:: mesh_dim_x + x, out, true);
	out << "];" << endl;

	// show MaxDelay matrix
	vector < vector < double > > md_mtx = getMaxDelayMtx();

	out << endl << "max_delay = [" << endl;
	for (unsigned int y = 0; y < md_mtx.size(); y++) 
	{
	    out << "   ";
	    for (unsigned int x = 0; x < md_mtx[y].size(); x++)
		out << setw(6) << md_mtx[y][x];
	    out << endl;
	}
	out << "];" << endl;

	// show RoutedFlits matrix
	vector < vector < unsigned long > > rf_mtx = getRoutedFlitsMtx();

	out << endl << "routed_flits = [" << endl;
	for (unsigned int y = 0; y < rf_mtx.size(); y++) 
	{
	    out << "   ";
	    for (unsigned int x = 0; x < rf_mtx[y].size(); x++)
		out << setw(10) << rf_mtx[y][x];
	    out << endl;
	}
	out << "];" << endl;

	showPowerBreakDown(out);
	showPowerManagerStats(out);
    }

#ifdef DEBUG

    if (GlobalParams::topology == TOPOLOGY_MESH)
    {
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
		out << "PE["<<x << "," << y<< "]" << noc->t[x][y]->pe->getQueueSize()<< ",";
    }
    else // other delta topologies
    {
	out << "Queue sizes: " ;
	for (int i=0;i<GlobalParams::n_delta_tiles;i++)
		out << "PE"<<i << ": " << noc->core[i]->pe->getQueueSize()<< ",";
	out << endl;
    }
	
    out << endl;
#endif
	// Add in showStats() method after other output lines
	out << "% Traffic mode: " << (GlobalParams::traffic_in_bytes ? "BYTES" : "DATA_VOLUME") << endl;
	if (GlobalParams::traffic_in_bytes)
    out << "% Flit size: " << GlobalParams::flit_size << " bits (" << (GlobalParams::flit_size/8) << " bytes)" << endl;
    //int total_cycles = GlobalParams::simulation_time - GlobalParams::stats_warm_up_time;
    out << "% Total received packets: " << getReceivedPackets() << endl;
    out << "% Total received flits: " << getReceivedFlits() << endl;
    out << "% Received/Ideal flits Ratio: " << getReceivedIdealFlitRatio() << endl;
    out << "% Average wireless utilization: " << getWirelessPackets()/(double)getReceivedPackets() << endl;
    out << "% Global average delay (cycles): " << getAverageDelay() << endl;
    out << "% Max delay (cycles): " << getMaxDelay() << endl;
    out << "% Network throughput (flits/cycle): " << getAggregatedThroughput() << endl;
    out << "% \tNetwork throughput (Gbps): " << getNetworkThroughputGbps() << endl;
    out << "% \tNetwork throughput (GB/s): " << getNetworkThroughputGBps() << endl;
    out << "% Average IP throughput (flits/cycle/IP): " << getThroughput() << endl;
    out << "% \tAverage IP throughput (Gbps): " << getIPThroughputGbps() << endl;
    out << "% \tAverage IP throughput (GB/s): " << getIPThroughputGBps() << endl;
    out << "% Total energy (J): " << getTotalPower() << endl;
    out << "% \tDynamic energy (J): " << getDynamicPower() << endl;
    out << "% \tStatic energy (J): " << getStaticPower() << endl;

    // true throughput stats
    out << "%" << endl;
    out << "% === True Throughput (based on actual traffic completion time) ===" << endl;
    out << "% Actual simulation end time (cycles): " << getActualSimulationEndTime() << endl;
    out << "% True Network throughput (flits/cycle): " << getTrueAggregatedThroughput() << endl;
    out << "% \tTrue Network throughput (Gbps): " << getTrueNetworkThroughputGbps() << endl;
    out << "% \tTrue Network throughput (GB/s): " << getTrueNetworkThroughputGBps() << endl;
    out << "% True Average IP throughput (flits/cycle/IP): " << getTrueThroughput() << endl;
    out << "% \tTrue Average IP throughput (Gbps): " << getTrueIPThroughputGbps() << endl;
    out << "% \tTrue Average IP throughput (GB/s): " << getTrueIPThroughputGBps() << endl;
	
	// Collect and show stall statistics
	collectStallStats();
	out << "%" << endl;
	generateStallHeatmap(out);
	out << "%" << endl;
	showStallStats(out);
	out << "%" << endl;
    showStallsByReasonPerNode(out);

    // Show traffic table stats
    if (GlobalParams::traffic_distribution == TRAFFIC_COMMUNICATION_TABLE) {
        showTrafficCompletionStats(out);
        showTrafficTimingStats(out); // Add timing statistics report
        
        // Generate detailed timing data files
        if (detailed) {
            string results_path = GlobalParams::stats_warm_up_time > 0 ? 
                "results/timing_data/" : "results/timing_data_warm/";
            
            // Create directory if it doesn't exist
            string mkdir_cmd = "mkdir -p " + results_path;
            int dir_result = system(mkdir_cmd.c_str());
            if (dir_result != 0) {
                cout << "Warning: Failed to create directory " << results_path << endl;
            }
            
            // Export CSV data for visualization
            string csv_filename = results_path + "task_timings.csv";
            exportTaskTimingData(csv_filename);
            
            // Generate histogram data
            generateHistogramData();
        }
    }
    if (GlobalParams::show_buffer_stats)
      showBufferStats(out);

}

void GlobalStats::updatePowerBreakDown(map<string,double> &dst,PowerBreakdown* src)
{
    for (int i=0;i!=src->size;i++)
    {
		dst[src->breakdown[i].label]+=src->breakdown[i].value;
    }
}

void GlobalStats::showPowerManagerStats(std::ostream & out)
{
    std::streamsize p = out.precision();
    int total_cycles = sc_time_stamp().to_double() / GlobalParams::clock_period_ps - GlobalParams::reset_time;

    out.precision(4);

    out << "powermanager_stats_tx = [" << endl;
    out << "%\tFraction of: TX Transceiver off (TTXoff), AntennaBufferTX off (ABTXoff) " << endl;
    out << "%\tHUB\tTTXoff\tABTXoff\t" << endl;

    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
            it != GlobalParams::hub_configuration.end();
            ++it)
    {
	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	out << "\t" << hub_id << "\t" << std::fixed << (double)h->total_ttxoff_cycles/total_cycles << "\t";

	int s = 0;
	for (map<int,int>::iterator i = h->abtxoff_cycles.begin(); i!=h->abtxoff_cycles.end();i++) s+=i->second;

	out << (double)s/h->abtxoff_cycles.size()/total_cycles << endl;
    }

    out << "];" << endl;



    out << "powermanager_stats_rx = [" << endl;
    out << "%\tFraction of: RX Transceiver off (TRXoff), AntennaBufferRX off (ABRXoff), BufferToTile off (BTToff) " << endl;
    out << "%\tHUB\tTRXoff\tABRXoff\tBTToff\t" << endl;



    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
            it != GlobalParams::hub_configuration.end();
            ++it)
    {
	string bttoff_str;

	out.precision(4);

	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	out << "\t" << hub_id << "\t" << std::fixed << (double)h->total_sleep_cycles/total_cycles << "\t";

	int s = 0;
	for (map<int,int>::iterator i = h->buffer_rx_sleep_cycles.begin();
		i!=h->buffer_rx_sleep_cycles.end();i++)
	    s+=i->second;

	out << (double)s/h->buffer_rx_sleep_cycles.size()/total_cycles << "\t";

	s = 0;
	for (map<int,int>::iterator i = h->buffer_to_tile_poweroff_cycles.begin();
		i!=h->buffer_to_tile_poweroff_cycles.end();i++)
	{
	    double bttoff_fraction = i->second/(double)total_cycles;
	    s+=i->second;
	    if (bttoff_fraction<0.25)
		bttoff_str+=" ";
	    else if (bttoff_fraction<0.5)
		    bttoff_str+=".";
	    else if (bttoff_fraction<0.75)
		    bttoff_str+="o";
	    else if (bttoff_fraction<0.90)
		    bttoff_str+="O";
	    else 
		bttoff_str+="0";
	    

	}
	out << (double)s/h->buffer_to_tile_poweroff_cycles.size()/total_cycles << "\t" << bttoff_str << endl;
    }

    out << "];" << endl;

    out.unsetf(std::ios::fixed);

    out.precision(p);

}

void GlobalStats::showPowerBreakDown(std::ostream & out)
{
    map<string,double> power_dynamic;
    map<string,double> power_static;

    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    {
		updatePowerBreakDown(power_dynamic, noc->t[x][y]->r->power.getDynamicPowerBreakDown());
		updatePowerBreakDown(power_static, noc->t[x][y]->r->power.getStaticPowerBreakDown());
	    }
    }
    else // other delta topologies
    {
	for (int y = 0; y < GlobalParams::n_delta_tiles; y++)
	{
	    updatePowerBreakDown(power_dynamic, noc->core[y]->r->power.getDynamicPowerBreakDown());
	    updatePowerBreakDown(power_static, noc->core[y]->r->power.getStaticPowerBreakDown());
	}
    }

    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
	    it != GlobalParams::hub_configuration.end();
	    ++it)
    {
	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	updatePowerBreakDown(power_dynamic, 
		h->power.getDynamicPowerBreakDown());

	updatePowerBreakDown(power_static, 
		h->power.getStaticPowerBreakDown());
    }

    printMap("power_dynamic",power_dynamic,out);
    printMap("power_static",power_static,out);

}



void GlobalStats::showBufferStats(std::ostream & out)
{
  out << "Router id\tBuffer N\t\tBuffer E\t\tBuffer S\t\tBuffer W\t\tBuffer L" << endl;
  out << "         \tMean\tMax\tMean\tMax\tMean\tMax\tMean\tMax\tMean\tMax" << endl;
  
  if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
    	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
    	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
      	{
			out << noc->t[x][y]->r->local_id;
			noc->t[x][y]->r->ShowBuffersStats(out);
			out << endl;
     	}
    }
    else // other delta topologies
    {
    	for (int y = 0; y < GlobalParams::n_delta_tiles; y++)
    	{
			out << noc->core[y]->r->local_id;
			noc->core[y]->r->ShowBuffersStats(out);
			out << endl;
     	}
    }

}

double GlobalStats::getReceivedIdealFlitRatio()
{
    int total_cycles;
    total_cycles= GlobalParams::simulation_time - GlobalParams::stats_warm_up_time;
    double ratio;
    if (GlobalParams::topology == TOPOLOGY_MESH) 
    {
	ratio = getReceivedFlits() /(GlobalParams::packet_injection_rate * (GlobalParams::min_packet_size +
		    GlobalParams::max_packet_size)/2 * total_cycles * GlobalParams::mesh_dim_y * GlobalParams::mesh_dim_x);
    }
    else // other delta topologies
    {
	ratio = getReceivedFlits() /(GlobalParams::packet_injection_rate * (GlobalParams::min_packet_size +
		    GlobalParams::max_packet_size)/2 * total_cycles * GlobalParams::n_delta_tiles);
    }
    return ratio;
}

double GlobalStats::getNetworkThroughputGbps()
{
    // Calculate clock frequency in GHz from clock period in ps
    double clock_frequency_GHz = 1000.0 / GlobalParams::clock_period_ps;
    
    // Convert flits/cycle to Gbps
    double network_throughput_Gbps = getAggregatedThroughput() * GlobalParams::flit_size * clock_frequency_GHz;
    
    return network_throughput_Gbps;
}

double GlobalStats::getNetworkThroughputGBps()
{
    // Convert Gbps to GB/s (divide by 8)
    return getNetworkThroughputGbps() / 8.0;
}

double GlobalStats::getIPThroughputGbps()
{
    // Calculate clock frequency in GHz from clock period in ps
    double clock_frequency_GHz = 1000.0 / GlobalParams::clock_period_ps;
    
    // Convert flits/cycle/IP to Gbps
    double ip_throughput_Gbps = getThroughput() * GlobalParams::flit_size * clock_frequency_GHz;
    
    return ip_throughput_Gbps;
}

double GlobalStats::getIPThroughputGBps()
{
    // Convert Gbps to GB/s (divide by 8)
    return getIPThroughputGbps() / 8.0;
}

void GlobalStats::collectStallStats() {
    // Initialize matrices
    // For each router:
    //   1. Collect stall counts by direction
    //   2. Collect stall counts by reason
    //   3. Update heatmap matrices

    // Create matrices for storing stall data
    vector<vector<unsigned long>> pe_to_router_stalls;
    vector<vector<unsigned long>> router_to_router_stalls;
    vector<vector<unsigned long>> total_stalls;
    
    // Direction-specific aggregates
    unsigned long pe_to_router_stalls_by_direction[DIRECTIONS + 2] = {0};
    unsigned long router_to_router_stalls_by_direction[DIRECTIONS + 2] = {0};
    
    // Reason-specific aggregates
    unsigned long total_buffer_full_stalls = 0;
    unsigned long total_reservation_stalls = 0;
    unsigned long total_vc_busy_stalls = 0;
	unsigned long total_already_reserved_stalls = 0;

    
    // Initialize matrices based on topology
    if (GlobalParams::topology == TOPOLOGY_MESH) {

        pe_to_router_stalls.resize(GlobalParams::mesh_dim_y);
        router_to_router_stalls.resize(GlobalParams::mesh_dim_y);
        total_stalls.resize(GlobalParams::mesh_dim_y);
        
        for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
            pe_to_router_stalls[y].resize(GlobalParams::mesh_dim_x, 0);
            router_to_router_stalls[y].resize(GlobalParams::mesh_dim_x, 0);
            total_stalls[y].resize(GlobalParams::mesh_dim_x, 0);
        }
        // PRF
        /*
        // Collect data from each router in the mesh
        for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
            for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
                Router* router = noc->t[x][y]->r;
                
                // Collect stalls by direction
                for (int dir = 0; dir < DIRECTIONS + 2; dir++) {
                    pe_to_router_stalls_by_direction[dir] += router->stall_stats.pe_to_router_stalls[dir];
                    router_to_router_stalls_by_direction[dir] += router->stall_stats.router_to_router_stalls[dir];
                    
                    // Add to position-based matrices
                    pe_to_router_stalls[y][x] += router->stall_stats.pe_to_router_stalls[dir];
                    router_to_router_stalls[y][x] += router->stall_stats.router_to_router_stalls[dir];
                }
                
                // Collect stalls by reason
                total_buffer_full_stalls += router->stall_stats.buffer_full_stalls;
                total_reservation_stalls += router->stall_stats.reservation_stalls;
                total_vc_busy_stalls += router->stall_stats.vc_busy_stalls;
				total_already_reserved_stalls += router->stall_stats.already_reserved_stalls;
                
                // Calculate total stalls for heatmap
                total_stalls[y][x] = pe_to_router_stalls[y][x] + router_to_router_stalls[y][x];
            }
        }
        */
       for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
        for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
            Router* router = noc->t[x][y]->r;
            
            // Collect all statistics in a single pass
            for (int dir = 0; dir < DIRECTIONS + 2; dir++) {
                unsigned long pe_stalls = router->stall_stats.pe_to_router_stalls[dir];
                unsigned long router_stalls = router->stall_stats.router_to_router_stalls[dir];
                
                // Update direction-specific aggregates
                pe_to_router_stalls_by_direction[dir] += pe_stalls;
                router_to_router_stalls_by_direction[dir] += router_stalls;
                
                // Update position-based matrices
                pe_to_router_stalls[y][x] += pe_stalls;
                router_to_router_stalls[y][x] += router_stalls;
            }
            
            // Collect stalls by reason in the same loop
            total_buffer_full_stalls += router->stall_stats.buffer_full_stalls;
            total_reservation_stalls += router->stall_stats.reservation_stalls;
            total_vc_busy_stalls += router->stall_stats.vc_busy_stalls;
            total_already_reserved_stalls += router->stall_stats.already_reserved_stalls;
            
            // Calculate total stalls once
            total_stalls[y][x] = pe_to_router_stalls[y][x] + router_to_router_stalls[y][x];
        }
    }
    
    }
    else { // Delta topologies
        cout << "Delta not supported at the moemnt" << endl;
    }
    
    // Store the collected statistics
    stall_matrices.pe_to_router_stalls = pe_to_router_stalls;
    stall_matrices.router_to_router_stalls = router_to_router_stalls;
    stall_matrices.total_stalls = total_stalls;
    
    // Store direction aggregates
    for (int dir = 0; dir < DIRECTIONS + 2; dir++) {
        stall_stats.pe_to_router_stalls_by_direction[dir] = pe_to_router_stalls_by_direction[dir];
        stall_stats.router_to_router_stalls_by_direction[dir] = router_to_router_stalls_by_direction[dir];
    }
    
    // Store reason aggregates
    stall_stats.buffer_full_stalls = total_buffer_full_stalls;
    stall_stats.reservation_stalls = total_reservation_stalls;
    stall_stats.vc_busy_stalls = total_vc_busy_stalls;
	stall_stats.already_reserved_stalls = total_already_reserved_stalls;
}

void GlobalStats::showStallStats(std::ostream & out) {
    // Calculate total stalls
    unsigned long total_pe_router_stalls = 0;
    unsigned long total_router_router_stalls = 0;
    
    for (int dir = 0; dir < DIRECTIONS + 2; dir++) {
        total_pe_router_stalls += stall_stats.pe_to_router_stalls_by_direction[dir];
        total_router_router_stalls += stall_stats.router_to_router_stalls_by_direction[dir];
    }
    
    unsigned long total_stalls = total_pe_router_stalls + total_router_router_stalls;
    
    // Print summary
    out << "% === Stall Statistics Summary ===" << endl;
    out << "% Total stalls: " << total_stalls << endl;
    out << "% PE→Router stalls: " << total_pe_router_stalls 
        << " (" << fixed << setprecision(2) 
        << (total_stalls > 0 ? 100.0 * total_pe_router_stalls / total_stalls : 0) << "%)" << endl;
    out << "% Router→Router stalls: " << total_router_router_stalls 
        << " (" << fixed << setprecision(2) 
        << (total_stalls > 0 ? 100.0 * total_router_router_stalls / total_stalls : 0) << "%)" << endl;
    
    // Print stalls by reason
    out << "%" << endl;
    out << "% === Stalls by Reason ===" << endl;
    out << "% Buffer full stalls: " << stall_stats.buffer_full_stalls 
        << " (" << fixed << setprecision(2) 
        << (total_stalls > 0 ? 100.0 * stall_stats.buffer_full_stalls / total_stalls : 0) << "%)" << endl;
    out << "% VC busy stalls: " << stall_stats.vc_busy_stalls 
        << " (" << fixed << setprecision(2) 
        << (total_stalls > 0 ? 100.0 * stall_stats.vc_busy_stalls / total_stalls : 0) << "%)" << endl;
    out << "% Reservation stalls: " << stall_stats.reservation_stalls 
        << " (" << fixed << setprecision(2) 
        << (total_stalls > 0 ? 100.0 * stall_stats.reservation_stalls / total_stalls : 0) << "%)" << endl;
	out << "% Already reserved stalls: " << stall_stats.already_reserved_stalls 
		<< " (" << fixed << setprecision(2) 
		<< (total_stalls > 0 ? 100.0 * stall_stats.already_reserved_stalls / total_stalls : 0) << "%)" << endl;
    
    // Print stalls by direction
    out << "%" << endl;
    out << "% === Stalls by Direction ===" << endl;
    out << "% Direction\tPE→Router\t\tRouter→Router" << endl;
    
    string direction_names[DIRECTIONS + 2] = {"North", "East", "South", "West", "Local", "Hub"};
    
    for (int dir = 0; dir < DIRECTIONS + 2; dir++) {
        double pe_router_percent = total_pe_router_stalls > 0 ? 
            100.0 * stall_stats.pe_to_router_stalls_by_direction[dir] / total_pe_router_stalls : 0;
        
        double router_router_percent = total_router_router_stalls > 0 ?
            100.0 * stall_stats.router_to_router_stalls_by_direction[dir] / total_router_router_stalls : 0;
        
        out << "% " << setw(10) << direction_names[dir] 
            << "\t" << stall_stats.pe_to_router_stalls_by_direction[dir] 
            << " (" << fixed << setprecision(2) << pe_router_percent << "%)"
            << "\t\t" << stall_stats.router_to_router_stalls_by_direction[dir] 
            << " (" << fixed << setprecision(2) << router_router_percent << "%)" 
            << endl;
    }
}

void GlobalStats::generateStallHeatmap(std::ostream & out) {
    if (GlobalParams::topology != TOPOLOGY_MESH) {
        out << "% Stall heatmap is only available for mesh topology" << endl;
        return;
    }
    
    out << "% === Stall Heatmap ===" << endl;
    out << "% Each cell shows total stalls (PE→Router + Router→Router)" << endl;
    out << "% Intensity: . < ░ < ▒ < ▓ < █" << endl;
    
    // Find maximum value for scaling
    unsigned long max_stalls = 0;
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
        for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
            if (stall_matrices.total_stalls[y][x] > max_stalls)
                max_stalls = stall_matrices.total_stalls[y][x];
        }
    }
    
    // Generate heatmap
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
        out << "% ";
        for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
            double intensity = max_stalls > 0 ? 
                (double)stall_matrices.total_stalls[y][x] / max_stalls : 0;
            
            if (intensity == 0) out << " . ";
            else if (intensity < 0.2) out << " . ";
            else if (intensity < 0.4) out << " ░ ";
            else if (intensity < 0.6) out << " ▒ ";
            else if (intensity < 0.8) out << " ▓ ";
            else out << " █ ";
        }
        out << endl;
    }
    
    // Generate numeric heatmap
    out << "% " << endl;
    out << "% Raw stall counts:" << endl;
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
        out << "% ";
        for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
            out << setw(8) << stall_matrices.total_stalls[y][x] << " ";
        }
        out << endl;
    }
}

void GlobalStats::showStallsByReasonPerNode(std::ostream & out) {
    out << "%" << endl;
    out << "% === Per-Node Stall Analysis ===" << endl;
    
    // Header
    out << "% Node ID\tBuffer Full\tVC Busy\tReservation\tAlready Reserved\tTotal" << endl;
    
    if (GlobalParams::topology == TOPOLOGY_MESH) {
        for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
            for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
                Router* router = noc->t[x][y]->r;
                int id = y * GlobalParams::mesh_dim_x + x;
                
                // Get stall counts by reason for this router
                unsigned long buf_full = router->stall_stats.buffer_full_stalls;
                unsigned long vc_busy = router->stall_stats.vc_busy_stalls;
                unsigned long resv = router->stall_stats.reservation_stalls;
                unsigned long already = router->stall_stats.already_reserved_stalls;
                unsigned long total = buf_full + vc_busy + resv + already;
                
                // Output formatted row
                out << "% " << setw(7) << id << "\t" 
                    << setw(11) << buf_full << "\t"
                    << setw(7) << vc_busy << "\t"
                    << setw(11) << resv << "\t"
                    << setw(16) << already << "\t"
                    << setw(5) << total << endl;
            }
        }
    } else {
        out << "Stall breakdown by node Not supported for other topologies!" << endl;
    }
}



void GlobalStats::showTrafficCompletionStats(std::ostream & out)
{
    out << "%" << endl;
    out << "% === Traffic Completion Statistics ===" << endl;
    out << "% TaskID\tUsed?" << endl;
    out << "% ---------------------" << endl;
    
    int total_tasks = 0;
    int completed_tasks = 0;

    if (traffic_communication_table == nullptr)
        out << "% Traffic table not available - null pointer" << endl;

    // Access the traffic communication table through GlobalParams::traffic_table
    const auto& traffic_table = traffic_communication_table->getTCommunicationTable();
    
    for (const auto& comm : traffic_table) {
        out << "% " << setw(6) << comm.taskID << "\t" 
            << (comm.traffic_used ? "YES" : "NO") << endl;
        total_tasks++;
        if (comm.traffic_used) completed_tasks++;
    }
    
    out << "% ---------------------" << endl;
    out << "% Completion Rate: " << completed_tasks << "/" << total_tasks 
         << " (" << fixed << setprecision(2)
         << (total_tasks > 0 ? 100.0 * completed_tasks / total_tasks : 0)
         << "%)" << endl;
    out << "% =============================================" << endl;
}

void GlobalStats::showTrafficTimingStats(std::ostream & out) {
    out << "%" << endl;
    out << "% === Traffic Processing Time Statistics ===" << endl;
    out << "% TaskID | Layer | Wait Time | Transmit Time | Compute Time | Receive Time | Total Time | Valid?" << endl;
    out << "% --------------------------------------------------------------------------------------" << endl;
    
    // Get traffic table
    vector<TrafficCommunication> traffic_table;
    traffic_table = traffic_communication_table->getTCommunicationTable();
    
    unsigned long long total_wait_time = 0;
    unsigned long long total_transmit_time = 0;
    unsigned long long total_compute_time = 0;
    unsigned long long total_receive_time = 0;
    unsigned long long total_processing_time = 0;
    int valid_count = 0;
    
    for (const auto& comm : traffic_table) {
        // if (comm.timing_valid) {
            unsigned long long wait_time = comm.wait_end_cycle - comm.wait_start_cycle;
            unsigned long long transmit_time = comm.transmit_end_cycle - comm.transmit_start_cycle;
            unsigned long long compute_time = comm.compute_end_cycle - comm.compute_start_cycle;
            unsigned long long receive_time = comm.receive_end_cycle - comm.receive_start_cycle;
            unsigned long long total_time = comm.receive_end_cycle - comm.wait_start_cycle;
            
            out << "% " << setw(6) << comm.taskID << " | "
                << setw(5) << comm.layerID << " | " 
                << setw(9) << wait_time << " | "
                << setw(13) << transmit_time << " | "
                << setw(12) << compute_time << " | "
                << setw(12) << receive_time << " | "
                << setw(10) << total_time << " | "
                << (comm.timing_valid ? "YES" : "NO") << endl;
            
            total_wait_time += wait_time;
            total_transmit_time += transmit_time;
            total_compute_time += compute_time;
            total_receive_time += receive_time;
            total_processing_time += total_time;
            valid_count++;
        // }
    }
    
    out << "% --------------------------------------------------------------------------------------" << endl;
    // if (valid_count > 0) {
        out << "% Average  |       | " 
            << setw(9) << (double)total_wait_time / valid_count << " | "
            << setw(13) << (double)total_transmit_time / valid_count << " | "
            << setw(12) << (double)total_compute_time / valid_count << " | "
            << setw(12) << (double)total_receive_time / valid_count << " | "
            << setw(10) << (double)total_processing_time / valid_count << " |      " << endl;
    // }
    out << "% Total Tasks with Complete Timing: " << valid_count << endl;
}

void GlobalStats::exportTaskTimingData(const string& filename) {
    ofstream outfile(filename);
    
    // CSV header
    outfile << "task_id,layer_id,wait_start,wait_end,wait_duration,"
            << "transmit_start,transmit_end,transmit_duration,"
            << "compute_start,compute_end,compute_duration,"
            << "receive_start,receive_end,receive_duration,total_duration" << endl;
            
    // Get traffic table
    vector<TrafficCommunication> traffic_table = traffic_communication_table->getTCommunicationTable();
    
    for (const auto& comm : traffic_table) {
        // if (comm.timing_valid) {
            unsigned long long wait_duration = comm.wait_end_cycle - comm.wait_start_cycle;
            unsigned long long transmit_duration = comm.transmit_end_cycle - comm.transmit_start_cycle;
            unsigned long long compute_duration = comm.compute_end_cycle - comm.compute_start_cycle;
            unsigned long long receive_duration = comm.receive_end_cycle - comm.receive_start_cycle;
            unsigned long long total_duration = comm.receive_end_cycle - comm.wait_start_cycle;
            
            outfile << comm.taskID << "," << comm.layerID << ","
                    << comm.wait_start_cycle << "," << comm.wait_end_cycle << "," << wait_duration << ","
                    << comm.transmit_start_cycle << "," << comm.transmit_end_cycle << "," << transmit_duration << ","
                    << comm.compute_start_cycle << "," << comm.compute_end_cycle << "," << compute_duration << ","
                    << comm.receive_start_cycle << "," << comm.receive_end_cycle << "," << receive_duration << ","
                    << total_duration << endl;
        // }
    }
    
    outfile.close();
}

void GlobalStats::generateHistogramData() {
    // Get traffic table
    vector<TrafficCommunication> traffic_table = traffic_communication_table->getTCommunicationTable();
    
    // Determine output directory - same logic as in showStats()
    string results_path = GlobalParams::stats_warm_up_time > 0 ? 
        "results/timing_data/" : "results/timing_data_warm/";
    
    // Create directory if it doesn't exist
    string mkdir_cmd = "mkdir -p " + results_path;
    int dir_result = system(mkdir_cmd.c_str());
    if (dir_result != 0) {
        cout << "Warning: Failed to create directory " << results_path << endl;
    }
    
    // Storage for histogram data
    vector<unsigned long long> wait_times;
    vector<unsigned long long> transmit_times;
    vector<unsigned long long> compute_times;
    vector<unsigned long long> receive_times;
    vector<unsigned long long> total_times;
    
    // Collect timing data
    for (const auto& comm : traffic_table) {
        // Add defensive checks to avoid bogus values
        unsigned long long wait_time = (comm.wait_end_cycle > comm.wait_start_cycle) ? 
                                      (comm.wait_end_cycle - comm.wait_start_cycle) : 0;
        
        unsigned long long transmit_time = (comm.transmit_end_cycle > comm.transmit_start_cycle) ? 
                                         (comm.transmit_end_cycle - comm.transmit_start_cycle) : 0;
        
        unsigned long long compute_time = (comm.compute_end_cycle > comm.compute_start_cycle) ? 
                                       (comm.compute_end_cycle - comm.compute_start_cycle) : 0;
        
        unsigned long long receive_time = (comm.receive_end_cycle > comm.receive_start_cycle) ? 
                                       (comm.receive_end_cycle - comm.receive_start_cycle) : 0;
        
        // Special handling for self-compute tasks
        if (comm.is_self_compute) {
            transmit_time = 0;
            receive_time = 0;
        }
        
        // Only include non-zero values to avoid skewing histograms
        if (wait_time > 0) wait_times.push_back(wait_time);
        if (transmit_time > 0) transmit_times.push_back(transmit_time);
        if (compute_time > 0) compute_times.push_back(compute_time);
        if (receive_time > 0) receive_times.push_back(receive_time);
        
        if (comm.receive_end_cycle > comm.wait_start_cycle) {
            total_times.push_back(comm.receive_end_cycle - comm.wait_start_cycle);
        }
    }
    
    // Output histogram data with properly pathed filenames
    outputHistogramData(results_path + "wait_time_histogram.csv", wait_times);
    outputHistogramData(results_path + "transmit_time_histogram.csv", transmit_times);
    outputHistogramData(results_path + "compute_time_histogram.csv", compute_times);
    outputHistogramData(results_path + "receive_time_histogram.csv", receive_times);
    outputHistogramData(results_path + "total_time_histogram.csv", total_times);
    
    cout << "Histogram data files generated in " << results_path << endl;
}

void GlobalStats::outputHistogramData(const string& filename, const vector<unsigned long long>& data) {
    if (data.empty()) return;
    
    // Calculate histogram bins
    unsigned long long min_val = *min_element(data.begin(), data.end());
    unsigned long long max_val = *max_element(data.begin(), data.end());
    
    // Create 20 bins
    const int num_bins = 20;
    unsigned long long bin_width = (max_val - min_val) / num_bins + 1;
    
    vector<int> histogram(num_bins, 0);
    
    // Populate histogram
    for (unsigned long long val : data) {
        int bin = (val - min_val) / bin_width;
        if (bin >= num_bins) bin = num_bins - 1;
        histogram[bin]++;
    }
    
    // Output histogram data
    ofstream outfile(filename);
    outfile << "bin_start,bin_end,count" << endl;
    
    for (int i = 0; i < num_bins; i++) {
        unsigned long long bin_start = min_val + i * bin_width;
        unsigned long long bin_end = bin_start + bin_width - 1;
        outfile << bin_start << "," << bin_end << "," << histogram[i] << endl;
    }
    
    outfile.close();
}

unsigned long long GlobalStats::getActualSimulationEndTime() {
    unsigned long long last_completion = 0;
    
    if (traffic_communication_table != nullptr) {
        const auto& traffic_table = traffic_communication_table->getTCommunicationTable();
        
        for (const auto& comm : traffic_table) {
            if (comm.receive_end_cycle > last_completion) {
                last_completion = comm.receive_end_cycle;
            }
        }
    }
    
    // If no valid timing data found, fall back to configured simulation time
    if (last_completion == 0) {
        return GlobalParams::simulation_time;
    }
    
    return last_completion;
}

double GlobalStats::getTrueAggregatedThroughput() {
    unsigned long long actual_end_time = getActualSimulationEndTime();
    unsigned long long effective_cycles = actual_end_time - GlobalParams::stats_warm_up_time;
    
    // Safety check to avoid division by zero
    if (effective_cycles <= 0) {
        return getAggregatedThroughput(); // Fall back to standard method
    }
    
    return (double)getReceivedFlits() / (double)effective_cycles;
}

double GlobalStats::getTrueThroughput()  {
    if (GlobalParams::topology == TOPOLOGY_MESH) {
        int number_of_ip = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
        return (double)getTrueAggregatedThroughput() / (double)(number_of_ip);
    }
    else { // other delta topologies
        int number_of_ip = GlobalParams::n_delta_tiles;
        return (double)getTrueAggregatedThroughput() / (double)(number_of_ip);
    }
}

double GlobalStats::getTrueNetworkThroughputGbps()  {
    double clock_frequency_GHz = 1000.0 / GlobalParams::clock_period_ps;
    return getTrueAggregatedThroughput() * GlobalParams::flit_size * clock_frequency_GHz;
}

double GlobalStats::getTrueNetworkThroughputGBps()  {
    return getTrueNetworkThroughputGbps() / 8.0;
}

double GlobalStats::getTrueIPThroughputGbps()  {
    double clock_frequency_GHz = 1000.0 / GlobalParams::clock_period_ps;
    return getTrueThroughput() * GlobalParams::flit_size * clock_frequency_GHz;
}

double GlobalStats::getTrueIPThroughputGBps()  {
    return getTrueIPThroughputGbps() / 8.0;
}