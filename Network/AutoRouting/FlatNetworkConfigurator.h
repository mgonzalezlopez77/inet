//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef __FLATNETWORKCONFIGURATOR_H__
#define __FLATNETWORKCONFIGURATOR_H__

#include <map>
#include <omnetpp.h>


/**
 * Configures IP addresses and routing tables for a "flat" network,
 * "flat" meaning that all hosts and routers will have the same
 * network address and will only differ in the host part.
 *
 * This module does't connect to any other modules (it has no gates),
 * and should have only one instance in the whole model. The module
 * will only run once, at the beginning of the simulation.
 * When it runs, it will:
 *    - #. assign IP addresses to hosts and routers. All hosts and
 *      routes will be in the same network (same network address).
 *      For simplicity, it will assign the same address to all interfaces
 *      of a router;
 *    - #. then it'll discover the topology of the network (using OMNeT++'s
 *      cTopology class), and calculate shortest paths;
 *    - #. finally, it will add routes which correspond to the shortest
 *      paths to the routing tables (see RoutingTable::addRoutingEntry()).
 *
 * To avoid interference with the above algorithm, it's recommended that
 * no host or router should have its address set explicitly, and
 * no routes are set up manually. Practically, routing files (.irt, .mrt)
 * should be missing or empty.
 *
 * All the above is executed in initialization stage 2. (In stage 0,
 * interfaces register themselves in the RoutingTable modules, and
 * in stage 1, routing files are read.)
 */
class FlatNetworkConfigurator : public cSimpleModule
{
  protected:
    struct RTEntry {
        int atAddress;
        int destAddress;

        bool operator<(const RTEntry& b) const {
           return atAddress<b.atAddress || (atAddress==b.atAddress && destAddress<b.destAddress);
        }
    };
    typedef std::map<RTEntry,int> RoutingTable; // (addr+destaddr) -> port

    RoutingTable rtable;

  public:
    Module_Class_Members(IP, cSimpleModule, 0);

  protected:
    virtual void numInitStages() const  {return 3;}
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
};

#endif

