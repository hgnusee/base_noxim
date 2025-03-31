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

GlobalStats::GlobalStats(const NoC * _noc)
{
    noc = _noc;

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
    out << "% Average IP throughput (flits/cycle/IP): " << getThroughput() << endl;
    out << "% Total energy (J): " << getTotalPower() << endl;
    out << "% \tDynamic energy (J): " << getDynamicPower() << endl;
    out << "% \tStatic energy (J): " << getStaticPower() << endl;
	
	// Collect and show stall statistics
	collectStallStats();
	out << "%" << endl;
	generateStallHeatmap(out);
	out << "%" << endl;
	showStallStats(out);
	out << "%" << endl;
    showStallsByReasonPerNode(out);

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
    }
    else { // Delta topologies
        int stg = log2(GlobalParams::n_delta_tiles);
        int sw = GlobalParams::n_delta_tiles/2; // switches per stage
        
        // Dimensions of delta switch network
        int dimX = stg; 
        int dimY = sw;
        
        // Resize matrices for delta topology
        pe_to_router_stalls.resize(GlobalParams::n_delta_tiles);
        router_to_router_stalls.resize(GlobalParams::n_delta_tiles);
        total_stalls.resize(GlobalParams::n_delta_tiles);
        
        for (int i = 0; i < GlobalParams::n_delta_tiles; i++) {
            pe_to_router_stalls[i].resize(1, 0);
            router_to_router_stalls[i].resize(1, 0);
            total_stalls[i].resize(1, 0);
        }
        
        // Collect stalls from processing elements
        for (int i = 0; i < GlobalParams::n_delta_tiles; i++) {
            Router* router = noc->core[i]->r;
            
            // Collect stalls by direction
            for (int dir = 0; dir < DIRECTIONS + 2; dir++) {
                pe_to_router_stalls_by_direction[dir] += router->stall_stats.pe_to_router_stalls[dir];
                router_to_router_stalls_by_direction[dir] += router->stall_stats.router_to_router_stalls[dir];
                
                pe_to_router_stalls[i][0] += router->stall_stats.pe_to_router_stalls[dir];
                router_to_router_stalls[i][0] += router->stall_stats.router_to_router_stalls[dir];
            }
            
            // Collect stalls by reason
            total_buffer_full_stalls += router->stall_stats.buffer_full_stalls;
            total_reservation_stalls += router->stall_stats.reservation_stalls;
            total_vc_busy_stalls += router->stall_stats.vc_busy_stalls;
			total_already_reserved_stalls += router->stall_stats.already_reserved_stalls;

            // Calculate total stalls for heatmap
            total_stalls[i][0] = pe_to_router_stalls[i][0] + router_to_router_stalls[i][0];
        }
        
        // Handle switches in delta networks
        for (int y = 0; y < dimY; y++) {
            for (int x = 0; x < dimX; x++) {
                Router* router = noc->t[x][y]->r;
                
                // Accumulate switch stalls to total counts
                for (int dir = 0; dir < DIRECTIONS + 2; dir++) {
                    pe_to_router_stalls_by_direction[dir] += router->stall_stats.pe_to_router_stalls[dir];
                    router_to_router_stalls_by_direction[dir] += router->stall_stats.router_to_router_stalls[dir];
                }
                
                total_buffer_full_stalls += router->stall_stats.buffer_full_stalls;
                total_reservation_stalls += router->stall_stats.reservation_stalls;
                total_vc_busy_stalls += router->stall_stats.vc_busy_stalls;
            }
        }
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