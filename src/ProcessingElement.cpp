/*
* Noxim - the NoC Simulator
*
* (C) 2005-2018 by the University of Catania
* For the complete list of authors refer to file ../doc/AUTHORS.txt
* For the license applied to these sources refer to file ../doc/LICENSE.txt
*
* This file contains the implementation of the processing element
*/

#include "ProcessingElement.h"

int ProcessingElement::randInt(int min, int max)
{
    return min +
    (int) ((double) (max - min + 1) * rand() / (RAND_MAX + 1.0));
}

/* rxProcess() acts in a state-machine-like mode PE_READY->PE_RECV->PE_READY (PE_BUSY not used)
When reset signal: 
    - set state = PE_READY
When during non-reset conditions, check following states:
    - when received first HEADER_FLIT, set state = PE_RECV [What if two HEAD Flit arrives back2back??]
    - go into case: PE_RECV
        > receive flits as normal and send to computeProcess(), and "store" in a queue, called flitRecvQ
            > for simplicity assume computeProcess() has NO DELAY
        > flitRecvQ is then able to be queried by packetShotbyPE() to decide if PE should shoot packet
            > flitRecvQ will extract 3 flits at once to be shot (could be more or less flits actually)
        > when TAIL_FLIT is received, set state = PE_READY
Need to have a counter function to keep track of:
    - recvBytes (based on incoming flit)
    - processedBytes (based on processsed flits for transmit)
    - totalBytes (based on totalVol data, to accumualte the amount of bytes received)
Exceptions to catch
    - Abishek: This wont happen since VC will reserve channel for packet from HEAD to TAIL
    - what if two HEAD_FLITs arrive back2back?? OR TAIL flit havent arrive but new HEAD comes
        > actually, not an issue, assume minVol is must hit for each incoming transaction source
        > && totalVol is the sum of the indiviudal totalVol 
        > at the end of day, as long as totalVol is correct then is fine
    
*/

void ProcessingElement::rxProcess()
{

    
    if (reset.read()) {
        ack_rx.write(0);
        current_level_rx = 0;
        // HG: reset ProcessingElement state flag to PE_WAIT
        state = PE_READY;
        currentTaskID = -1; // HG: initialize to -1 to indicate no taskID
        receivedTaskID = -1;
        last_recv_srcID = -1; // HG: initialize -1 to indicate no srcID yet
        recvBytes = 0;
        processedBytes = 0;
        sentBytes = 0;
        recv_totalBytes = 0;
        recv_minBytes = 0;
        compute_queue.clear(); // clear all compute queue pairs
    } else {
        // LOG << "In rxProcess() for PE " << local_id << endl;
        switch (state) {
            case PE_READY:
                if (req_rx.read() == 1 - current_level_rx) {
                    Flit flit_tmp = flit_rx.read();

                    current_level_rx = 1 - current_level_rx;	// Negate the old value for Alternating Bit Protocol (ABP)

                    if (flit_tmp.flit_type == FLIT_TYPE_HEAD) {
                        cout << "PE " << local_id << " received a HEAD flit. Begin RECEIVE!!" << endl;

                        
                        last_recv_srcID = flit_tmp.src_id; // set last_recv_srcID to srcID of received flit
                        receivedTaskID = flit_tmp.taskID;
                        

                        // reset recv/processed/sentBytes when receivedtaskID NOT same as currentTaskID
                        // currentTaskID is set to -1 during Reset or PE does not have a valid taskID
                        if (receivedTaskID != currentTaskID) {
                            cout << "PE " << local_id << " received a new taskID. Resetting data counters." << endl;
                            recvBytes = 0; // reset recvBytes to 0
                            processedBytes = 0; // reset processedBytes to 0
                            sentBytes = 0; // reset sentBytes to 0
                        }

                        recv_totalBytes = flit_tmp.total_vol; // set totalBytes to totalVol of received flit
                        recv_minBytes = flit_tmp.min_vol; // set minBytes to minVol of received flit
                        // currentTaskID = flit_tmp.taskID; // set currentTaskID to taskID of received flit

                        // start increment recvBytes, assume one flit is one byte
                        recvBytes ++;

                        if (recv_minBytes != -1) {
                        // run compute process delay if enough flits are received
                            if (flit_tmp.sequence_length == 1) {
                                // case where packet size is 1 (aka flit.sequence_length), no need to wait for more flits
                                computeProcess();
                                state = PE_READY;
                            } else {
                                state = PE_RECV; // Flag state to PE_RECV to begin receiveProcess() on next cycle
                                computeProcess();
                            }
                        } else {
                            // when min_vol = -1, set state PE_RECV and collect till TAIL flit
                            // no more computeProcess() needed
                            if (flit_tmp.sequence_length == 1) {
                                state = PE_READY;
                            } else {
                                state = PE_RECV;                                
                            }
                        }
                    }
                }
                ack_rx.write(current_level_rx);
            break;
            
            case PE_RECV:
            // PE_RECV State - Continue receive flits and sent to computeProcess()
            // TODO: For future implementation, store receive flits in a queue
                if (req_rx.read() == 1 - current_level_rx) {
                    Flit flit_tmp = flit_rx.read();

                    current_level_rx = 1 - current_level_rx;	// Negate the old value for Alternating Bit Protocol (ABP)
                    
                    receivedTaskID = flit_tmp.taskID;

                    if (recv_minBytes != -1) {
                        if (flit_tmp.flit_type == FLIT_TYPE_BODY) {
                            recvBytes ++;
                            computeProcess();
                        } else if (flit_tmp.flit_type == FLIT_TYPE_TAIL && receivedTaskID == currentTaskID && flit_tmp.waitID != -1) {
                            recvBytes ++;
                            computeProcess();
                        } else if (flit_tmp.flit_type == FLIT_TYPE_TAIL && receivedTaskID == currentTaskID && flit_tmp.waitID == -1) {
                            recvBytes ++;
                            computeProcess();
                            state = PE_READY;
                        } else if (flit_tmp.flit_type == FLIT_TYPE_HEAD) {
                            
                            // DO Nothing
    
                            // TODO: Consider many-to-one case, multiple PE come same PE
                            //  if flit is from different from currentTaskID, then ignore packet first
                            // state = PE_READY
                        }
                    } else {
                        recvBytes ++;
                    }

                    if (recvBytes >= recv_totalBytes) {
                        LOG << "PE " << local_id << " All Flits received. Stop receiving. " << endl;
                        state = PE_READY;
                    }
                }
                ack_rx.write(current_level_rx);
            break;
            
            case PE_BUSY:
                // PE_BUSY not used in this implementation
                computeProcess();
            break;

        }
    }
}

/* 
txProcess() checks if sufficient flits are available to be transmitted
    - if reset signal, reset all flags and counters
    - if during non-reset conditions, check of flit can be shot, increment processedBytes
    - if hit totalBytes == totalVol, then stop sending based on current traffic INFO
*/

void ProcessingElement::txProcess(void)
{
    if (reset.read()) {
        req_tx.write(0);
        current_level_tx = 0;
        transmittedAtPreviousCycle = false;
        tran_minBytes = 0;
        tran_totalBytes = 0;
    } else {
        Packet packet;
        // LOG << "In txProcess() for PE " << local_id << endl;
        if (canShot(packet)) {
            packet_queue.push(packet);
            transmittedAtPreviousCycle = true;
        } else
            transmittedAtPreviousCycle = false;

        if (ack_tx.read() == current_level_tx) {
            if (!packet_queue.empty()) {
                Flit flit = nextFlit();	// Generate a new flit

                LOG << "Flit created for PE" << local_id << " Type: " << 
                (flit.flit_type == FLIT_TYPE_HEAD ? "HEAD" : 
                flit.flit_type == FLIT_TYPE_TAIL ? "TAIL" : 
                flit.flit_type == FLIT_TYPE_BODY ? "BODY" : "UNKNOWN") <<
                " taskID = " << flit.taskID << " " << local_id << "->" << 
                flit.dst_id << " VC" << flit.vc_id << endl;

                flit_tx->write(flit);	// Send the generated flit
                current_level_tx = 1 - current_level_tx;	// Negate the old value for Alternating Bit Protocol (ABP)
                req_tx.write(current_level_tx);
            }
        }
    }
}

Flit ProcessingElement::nextFlit()
{
    Flit flit;
    Packet packet = packet_queue.front();

    flit.src_id = packet.src_id;
    flit.dst_id = packet.dst_id;
    flit.vc_id = packet.vc_id;
    flit.timestamp = packet.timestamp;
    flit.sequence_no = packet.size - packet.flit_left;
    flit.sequence_length = packet.size;
    flit.hop_no = 0;
    //  flit.payload     = DEFAULT_PAYLOAD;
    flit.taskID = packet.taskID; // HG: set taskID to flit based on packet

    flit.hub_relay_node = NOT_VALID;

    if (packet.size == packet.flit_left)
        flit.flit_type = FLIT_TYPE_HEAD;
    else if (packet.flit_left == 1)
        flit.flit_type = FLIT_TYPE_TAIL;
    else
        flit.flit_type = FLIT_TYPE_BODY;

    // HG: flag transaction trasmit is complete from src PE, when a tail flit is being created
    // go to traffic_communication_table and set trn_complete = true
    if (flit.flit_type == FLIT_TYPE_TAIL)
        traffic_communication_table->setTransmitComplete(packet.taskID, packet.src_id, packet.dst_id);
    
    // Encode total_vol and min_vol data to receive into HEAD Flit
    if (flit.flit_type == FLIT_TYPE_HEAD) {
        flit.total_vol = packet.dst_totalVol;
        flit.min_vol = packet.dst_minVol;
        flit.taskID = packet.taskID;
    }
        

    packet_queue.front().flit_left--;

    if (packet_queue.front().flit_left == 0)
        packet_queue.pop();

    return flit;
}

bool ProcessingElement::canShot(Packet & packet)
{
// assert(false);
    if(never_transmit) return false;

    //if(local_id!=16) return false;
    /* DEADLOCK TEST 
    double current_time = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

    if (current_time >= 4100) 
    {
        //if (current_time==3500)
            //cout << name() << " IN CODA " << packet_queue.size() << endl;
        return false;
    }
    //*/

#ifdef DEADLOCK_AVOIDANCE
    if (local_id%2==0)
    return false;
#endif
    bool shot;
    double threshold;

    double now = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

    //  HG: Traffic Comm Table-based, need to set TRAFFIC_COMMUNICATION_TABLE in config file
    if (GlobalParams::traffic_distribution == TRAFFIC_COMMUNICATION_TABLE) {
        if (never_transmit)
            return false;
        
        // get transaction for this PE from Traffic Communication Table
        TrafficCommunication& comm = traffic_communication_table->getTrafficCommunicationTable(local_id);

        if (comm.taskID == -1 && comm.src.empty() && comm.dst.empty() && comm.src_totalVol == 0 
            && comm.waitID.empty() && comm.waitOP == 0 && comm.traffic_used == true) {
                // cout << "No Traffic Communication Table found for src_id = " << local_id << endl;
                setCurrentTaskID(-1); // assume PE is not waiting for any taskID, and not in Operation
            return false;
        } else {
            shot = true;
            // HG: select virtual channels randomly
            int vc = randInt(0,GlobalParams::n_virtual_channels-1);
            
            // HG: set minBytes and totalBytes for this PE that is acting as a source
            tran_minBytes = comm.src_minVol;
            tran_totalBytes = comm.src_totalVol;
        
            
            // Extra check to stop sending this traffic if traffic has been used
            if (comm.traffic_used == true)
                return false;
            // set currentTaskID to waitID value of the current PE
            // FIX: Future Implementation, allow multiple waitID by just passing the whole waitID vector
            setCurrentTaskID(comm.waitID[0]);
            
            if (comm.src.size() == 1 && comm.dst.size() > 1) {
                // one-to-many case
                auto it = find(comm.trn_complete.begin(), comm.trn_complete.end(), TRN_WAIT);
                bool found_dst = (it != comm.trn_complete.end());
                if (found_dst == true)
                    return false;
                
                int dst_pos = distance(comm.trn_complete.begin(), it);
                comm.trn_complete[dst_pos] = TRN_BUSY;

                
                // if taskID no need wait for other PE, just send to dst
                if (comm.waitID[0] == -1){
                    // HG: make2() with values from comm object
                    packet.make2(comm.taskID, local_id, comm.dst[dst_pos], vc, now, 
                            comm.src_totalVol, comm.waitOP, comm.src_minVol, 
                            comm.src_totalVol, comm.dst_minVol, comm.dst_totalVol);
                    // waitID=-1 is a one-off transfer, flag this traffic as complete and dont use it anymore
                    traffic_communication_table->setTransmitComplete(comm.taskID, local_id, comm.dst[dst_pos]);
                } else if (comm.waitID[0] != -1) {
                    shot = packetShotbyPE(comm, local_id, packet);
                }

                LOG << "Packet created for PE" << local_id << " taskID = " << 
                comm.taskID << " " << local_id << "->" << comm.dst[dst_pos] << " VC" << vc << endl;

        } else if (comm.src.size() >= 1 && comm.dst.size() == 1) {
                // one-to-one & many-to-one case
                
                // if taskID no need wait for other PE, just send to dst
                if (comm.src_minVol == -1 && comm.waitID[0] == -1) {
                    // use comm.dst[0] as destination since is o2o or m2o case
                    packet.make2(comm.taskID, local_id, comm.dst[0], vc, now,
                        comm.src_totalVol, comm.waitOP, comm.src_minVol, comm.src_totalVol, 
                        comm.dst_minVol, comm.dst_totalVol);
                // waitID=-1 is a one-off transfer, flag this traffic as complete and dont use it anymore
                traffic_communication_table->setTransmitComplete(comm.taskID, local_id, comm.dst[0]);
                } else {
                    shot = packetShotbyPE(comm, local_id, packet);
                }
        }
            
        }
    } else if (GlobalParams::traffic_distribution != TRAFFIC_TABLE_BASED) {
    if (!transmittedAtPreviousCycle)
        threshold = GlobalParams::packet_injection_rate;
    else
        threshold = GlobalParams::probability_of_retransmission;

    shot = (((double) rand()) / RAND_MAX < threshold);
        if (shot) {
            if (GlobalParams::traffic_distribution == TRAFFIC_RANDOM)
                packet = trafficRandom();
            else if (GlobalParams::traffic_distribution == TRAFFIC_TRANSPOSE1)
                packet = trafficTranspose1();
            else if (GlobalParams::traffic_distribution == TRAFFIC_TRANSPOSE2)
                packet = trafficTranspose2();
            else if (GlobalParams::traffic_distribution == TRAFFIC_BIT_REVERSAL)
                packet = trafficBitReversal();
            else if (GlobalParams::traffic_distribution == TRAFFIC_SHUFFLE)
                packet = trafficShuffle();
            else if (GlobalParams::traffic_distribution == TRAFFIC_BUTTERFLY)
                packet = trafficButterfly();
            else if (GlobalParams::traffic_distribution == TRAFFIC_LOCAL)
                packet = trafficLocal();
            else if (GlobalParams::traffic_distribution == TRAFFIC_ULOCAL)
                packet = trafficULocal();
            else {
                cout << "Invalid traffic distribution: " << GlobalParams::traffic_distribution << endl;
                exit(-1);
            }
        }
    } else {			// Table based communication traffic
        if (never_transmit)
            return false;

        bool use_pir = (transmittedAtPreviousCycle == false);
        vector < pair < int, double > > dst_prob;
        double threshold =
            traffic_table->getCumulativePirPor(local_id, (int) now, use_pir, dst_prob);

        double prob = (double) rand() / RAND_MAX;
        shot = (prob < threshold);
        if (shot) {
            for (unsigned int i = 0; i < dst_prob.size(); i++) {
                if (prob < dst_prob[i].second) {
                            int vc = randInt(0,GlobalParams::n_virtual_channels-1);
                    packet.make(local_id, dst_prob[i].first, vc, now, getRandomSize());
                    break;
                }
            }
        }
    }
    
    return shot;
}

// Compute Process will need to check if recvbytes have reached minBytes
// if recvByte reach minBytes:
//  > hold the "byte" for one clock cycle delay and send on the next clock
//  > set processedBytes++
// if recvByte > minBytes, && recvByte mod minBytes == 0
//  > hold the "byte" for one clocl cycle delay and send on the next clock
// > set processedBytes++
// if processedBytes >= totalBytes
//  > set state = PE_READY
//  > stop processing more data
// How to hold byte for one or N clock cycle delay?
// >  push the recv_bytes into a vector array with N wait cycles, then we decrement it every cycle

void ProcessingElement::computeProcess()
{
    int compute_delayN = 1; // set fixed delay for compute process
    // FIX: compute_delayN > 1 is not working, no delay > 1 is modelled and last traffic not moving
    if (recvBytes == recv_minBytes || (recvBytes >= recv_minBytes && (recvBytes % recv_minBytes) == 0)) {
        // hold the "byte" for one clock cycle delay and send on the next clock
        compute_queue.push_back(make_pair(recvBytes, compute_delayN));
    }
    // Process completed computations
    auto it = compute_queue.begin();
    while (it != compute_queue.end()) {
        // Decrement delay counter
        it->second--;
        
        // When delay is complete, increment processedBytes and remove from queue
        if (it->second <= 0) {
            processedBytes += it->first;
            it = compute_queue.erase(it);
        } else {
            ++it;
        }
    }
    // Check if all processing is complete
    if (processedBytes >= recv_totalBytes && compute_queue.empty()) {
        state = PE_READY;
        LOG << "All Bytes Processed. Revert PE state --> PE_READY" << endl;
        // Notify traffic table that computation is complete
        traffic_communication_table->setComputeComplete(currentTaskID, last_recv_srcID, local_id);
    }
    /*     // HG: PE is in PE_BUSY state, stall PE from receiving new packets
        state = PE_BUSY;
        LOG << "PE " << local_id << " is in PE_BUSY state. PE Stalled.";
        LOG << " Compute cycle: " << compute_cycle << endl;
        compute_cycle--;

        if (compute_cycle == 0 && currentTaskID >= 0) {
            LOG << "PE " << local_id << " Compute Done!" << endl;
            state = PE_READY;
            LOG << "PE " << local_id << " is in PE_READY state. PE is ready to receive new packets." << endl;
            
            // set cmp_complete flag in Traffic Communication Table
            traffic_communication_table->setComputeComplete(currentTaskID, last_recv_srcID, local_id);
            // set currentTaskID to -1 to indicate no taskID
            currentTaskID = -1;
    } 
    */
}

int ProcessingElement::readyToSendBytes(){
    // calculate that are still needed to be sent
    return processedBytes - sentBytes;
}
void ProcessingElement::reservedTableMonitor()
{
    // HG: check and move transactions from reserved -> traffic comm table

    if (reset.read()){
        // DO nothing
    } else {
        // provide local_id, and move matches from resreved -> traffic comm
        traffic_communication_table->moveReserveToTrafficCommunicationTable(local_id);
    }
}

/* Applicable for all non-time0 packets (comm.size() > 0 && comm.waitID[0] != -1)
Check if arriving flits from rxProcess() for the following conditions:
- contains arriving flits from the previous cycle
&&
- contains nextID and its respective taskID that matches the taskID of the current local_id comm TrafficCommunication
&&
- contains waitID that matches the taskID of the current packet of interest

When the checks above all return true, do the folllowing:
- create packet to be sent, the packet size is based on the number of flits received
- increment the counter value of bytes_transmitted 
*/

bool ProcessingElement::packetShotbyPE(TrafficCommunication& comm, const int local_id, Packet& packet)
{
    // Check if comm.waitID is present in receivedTaskID, to prove that current PE is ready for this task
    auto it_find_waitID = find(comm.waitID.begin(), comm.waitID.end(), currentTaskID);
    bool found_waitID = (it_find_waitID != comm.waitID.end());
    
    if (comm.waitID[0] != -1 && found_waitID) {
        // Extra check to stop sending this traffic if traffic has been used or reach tran_totalBytes
        if (comm.traffic_used == true || sentBytes >= tran_totalBytes)
            return false;
        
        // Check if we have enough processed bytes to shoot packet
        if (readyToSendBytes() < tran_minBytes) {
            return false;
        }
        
        int vc = randInt(0, GlobalParams::n_virtual_channels-1);
        int now = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
        bool packet_created = false;
        
        if (comm.src.size() == 1 && comm.dst.size() > 1) {
            // One-to-many case: Find a destination that hasn't been processed yet
            auto it = find(comm.trn_complete.begin(), comm.trn_complete.end(), TRN_WAIT);
            if (it != comm.trn_complete.end()) {
                int dst_pos = distance(comm.trn_complete.begin(), it);
                comm.trn_complete[dst_pos] = TRN_BUSY;
                
                // Create packet for this destination
                packet.make2(comm.taskID, local_id, comm.dst[dst_pos], vc, now, 
                    1, comm.waitOP, comm.src_minVol, comm.src_totalVol, 
                    comm.dst_minVol, comm.dst_totalVol);
                
                // Update sentBytes and log
                sentBytes++;
                LOG << "Packet created for PE" << local_id << " taskID = " << 
                    comm.taskID << " " << local_id << "->" << comm.dst[dst_pos] << " VC" << vc << endl;
                packet_created = true;
            }
        } else {
            // One-to-one case: Process the single destination
            for (size_t i = 0; i < comm.dst.size(); i++) {
                packet.make2(comm.taskID, local_id, comm.dst[i], vc, now, 
                    1, comm.waitOP, comm.src_minVol, comm.src_totalVol, 
                    comm.dst_minVol, comm.dst_totalVol);
                
                sentBytes++;
                LOG << "Packet created for PE" << local_id << " taskID = " << 
                    comm.taskID << " " << local_id << "->" << comm.dst[i] << " VC" << vc << endl;
                packet_created = true;
                break; // Only one destination in this case
            }
        }
        
        // Check if we've sent all required bytes
        if (sentBytes >= tran_totalBytes) {
            state = PE_READY;
        }
        
        return packet_created;
    }
    
    return false;
}

void ProcessingElement::setCurrentTaskID(const int waitID){
    currentTaskID = waitID;
}

Packet ProcessingElement::trafficLocal()
{
    Packet p;
    p.src_id = local_id;
    double rnd = rand() / (double) RAND_MAX;

    vector<int> dst_set;

    int max_id = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y);

    for (int i=0;i<max_id;i++)
    {
    if (rnd<=GlobalParams::locality)
    {
        if (local_id!=i && sameRadioHub(local_id,i))
        dst_set.push_back(i);
    }
    else
        if (!sameRadioHub(local_id,i))
        dst_set.push_back(i);
    }


    int i_rnd = rand()%dst_set.size();

    p.dst_id = dst_set[i_rnd];
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    
    return p;
}


int ProcessingElement::findRandomDestination(int id, int hops)
{
    assert(GlobalParams::topology == TOPOLOGY_MESH);

    int inc_y = rand()%2?-1:1;
    int inc_x = rand()%2?-1:1;
    
    Coord current =  id2Coord(id);
    


    for (int h = 0; h<hops; h++)
    {

    if (current.x==0)
        if (inc_x<0) inc_x=0;

    if (current.x== GlobalParams::mesh_dim_x-1)
        if (inc_x>0) inc_x=0;

    if (current.y==0)
        if (inc_y<0) inc_y=0;

    if (current.y==GlobalParams::mesh_dim_y-1)
        if (inc_y>0) inc_y=0;

    if (rand()%2)
        current.x +=inc_x;
    else
        current.y +=inc_y;
    }
    return coord2Id(current);
}


int roulette()
{
    int slices = GlobalParams::mesh_dim_x + GlobalParams::mesh_dim_y -2;


    double r = rand()/(double)RAND_MAX;


    for (int i=1;i<=slices;i++)
    {
    if (r< (1-1/double(2<<i)))
    {
        return i;
    }
    }
    assert(false);
    return 1;
}


Packet ProcessingElement::trafficULocal()
{
    Packet p;
    p.src_id = local_id;

    int target_hops = roulette();

    p.dst_id = findRandomDestination(local_id,target_hops);

    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);

    return p;
}

Packet ProcessingElement::trafficRandom()
{
    Packet p;
    p.src_id = local_id;
    double rnd = rand() / (double) RAND_MAX;
    double range_start = 0.0;
    int max_id;

    if (GlobalParams::topology == TOPOLOGY_MESH)
    max_id = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) - 1; //Mesh 
    else    // other delta topologies
    max_id = GlobalParams::n_delta_tiles-1; 

    // Random destination distribution
    do {
    p.dst_id = randInt(0, max_id);

    // check for hotspot destination
    for (size_t i = 0; i < GlobalParams::hotspots.size(); i++) {

        if (rnd >= range_start && rnd < range_start + GlobalParams::hotspots[i].second) {
        if (local_id != GlobalParams::hotspots[i].first ) {
            p.dst_id = GlobalParams::hotspots[i].first;
        }
        break;
        } else
        range_start += GlobalParams::hotspots[i].second;	// try next
    }
#ifdef DEADLOCK_AVOIDANCE
    assert((GlobalParams::topology == TOPOLOGY_MESH));
    if (p.dst_id%2!=0)
    {
        p.dst_id = (p.dst_id+1)%256;
    }
#endif

    } while (p.dst_id == p.src_id);

    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);

    return p;
}
// TODO: for testing only
Packet ProcessingElement::trafficTest()
{
    Packet p;
    p.src_id = local_id;
    p.dst_id = 10;

    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);

    return p;
}

Packet ProcessingElement::trafficTranspose1()
{
    assert(GlobalParams::topology == TOPOLOGY_MESH);
    Packet p;
    p.src_id = local_id;
    Coord src, dst;

    // Transpose 1 destination distribution
    src.x = id2Coord(p.src_id).x;
    src.y = id2Coord(p.src_id).y;
    dst.x = GlobalParams::mesh_dim_x - 1 - src.y;
    dst.y = GlobalParams::mesh_dim_y - 1 - src.x;
    fixRanges(src, dst);
    p.dst_id = coord2Id(dst);

    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

Packet ProcessingElement::trafficTranspose2()
{
    assert(GlobalParams::topology == TOPOLOGY_MESH);
    Packet p;
    p.src_id = local_id;
    Coord src, dst;

    // Transpose 2 destination distribution
    src.x = id2Coord(p.src_id).x;
    src.y = id2Coord(p.src_id).y;
    dst.x = src.y;
    dst.y = src.x;
    fixRanges(src, dst);
    p.dst_id = coord2Id(dst);

    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

void ProcessingElement::setBit(int &x, int w, int v)
{
    int mask = 1 << w;

    if (v == 1)
    x = x | mask;
    else if (v == 0)
    x = x & ~mask;
    else
    assert(false);
}

int ProcessingElement::getBit(int x, int w)
{
    return (x >> w) & 1;
}

inline double ProcessingElement::log2ceil(double x)
{
    return ceil(log(x) / log(2.0));
}

Packet ProcessingElement::trafficBitReversal()
{

    int nbits =
    (int)
    log2ceil((double)
        (GlobalParams::mesh_dim_x *
        GlobalParams::mesh_dim_y));
    int dnode = 0;
    for (int i = 0; i < nbits; i++)
    setBit(dnode, i, getBit(local_id, nbits - i - 1));

    Packet p;
    p.src_id = local_id;
    p.dst_id = dnode;

    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

Packet ProcessingElement::trafficShuffle()
{

    int nbits =
    (int)
    log2ceil((double)
        (GlobalParams::mesh_dim_x *
        GlobalParams::mesh_dim_y));
    int dnode = 0;
    for (int i = 0; i < nbits - 1; i++)
    setBit(dnode, i + 1, getBit(local_id, i));
    setBit(dnode, 0, getBit(local_id, nbits - 1));

    Packet p;
    p.src_id = local_id;
    p.dst_id = dnode;

    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

Packet ProcessingElement::trafficButterfly()
{

    int nbits = (int) log2ceil((double)
        (GlobalParams::mesh_dim_x *
        GlobalParams::mesh_dim_y));
    int dnode = 0;
    for (int i = 1; i < nbits - 1; i++)
    setBit(dnode, i, getBit(local_id, i));
    setBit(dnode, 0, getBit(local_id, nbits - 1));
    setBit(dnode, nbits - 1, getBit(local_id, 0));

    Packet p;
    p.src_id = local_id;
    p.dst_id = dnode;

    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

void ProcessingElement::fixRanges(const Coord src,
                    Coord & dst)
{
    // Fix ranges
    if (dst.x < 0)
    dst.x = 0;
    if (dst.y < 0)
    dst.y = 0;
    if (dst.x >= GlobalParams::mesh_dim_x)
    dst.x = GlobalParams::mesh_dim_x - 1;
    if (dst.y >= GlobalParams::mesh_dim_y)
    dst.y = GlobalParams::mesh_dim_y - 1;
}

int ProcessingElement::getRandomSize()
{
    return randInt(GlobalParams::min_packet_size,
        GlobalParams::max_packet_size);
}

unsigned int ProcessingElement::getQueueSize() const
{
    return packet_queue.size();
}

