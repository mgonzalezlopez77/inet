/*
 * Copyright (C) 2004 Andras Varga
 * Copyright (C) 2008 Alfonso Ariza Quintana (global arp)
 * Copyright (C) 2014 OpenSim Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/networklayer/arp/ipv4/GlobalArp.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"

namespace inet {

Define_Module(GlobalArp);

GlobalArp::ArpCache GlobalArp::globalArpCache;
int GlobalArp::globalArpCacheRefCnt = 0;

static std::ostream& operator<<(std::ostream& out, const GlobalArp::ArpCacheEntry& e)
{
    return out << "MAC:" << e.macAddress;
}

GlobalArp::GlobalArp()
{
    if (++globalArpCacheRefCnt == 1) {
        if (!globalArpCache.empty())
            throw cRuntimeError("Global ARP cache not empty, model error in previous run?");
    }

    interfaceTable = nullptr;
}

GlobalArp::~GlobalArp()
{
    --globalArpCacheRefCnt;
    // delete my entries from the globalArpCache
    for (auto it = globalArpCache.begin(); it != globalArpCache.end(); ) {
        if (it->second->owner == this) {
            auto cur = it++;
            delete cur->second;
            globalArpCache.erase(cur);
        }
        else
            ++it;
    }
}

void GlobalArp::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        WATCH_PTRMAP(globalArpCache);
    }
    else if (stage == INITSTAGE_NETWORK_LAYER_3) {    // IP addresses should be available
        interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        isUp = isNodeUp();
        // register our addresses in the global cache
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
            InterfaceEntry *ie = interfaceTable->getInterface(i);
            if (ie->isLoopback())
                continue;
            if (!ie->ipv4Data())
                continue;
            Ipv4Address nextHopAddr = ie->ipv4Data()->getIPAddress();
            if (nextHopAddr.isUnspecified())
                continue; // if the address is not defined it isn't included in the global cache
            // check if the entry exist
            auto it = globalArpCache.find(nextHopAddr);
            if (it != globalArpCache.end())
                continue;
            ArpCacheEntry *entry = new ArpCacheEntry();
            entry->owner = this;
            entry->interfaceEntry = ie;
            entry->macAddress = ie->getMacAddress();

            auto where = globalArpCache.insert(globalArpCache.begin(), std::make_pair(nextHopAddr, entry));
            ASSERT(where->second == entry);
        }
        cModule *host = findContainingNode(this);
        if (host != nullptr)
            host->subscribe(interfaceIpv4ConfigChangedSignal, this);
    }
}

void GlobalArp::handleMessage(cMessage *msg)
{
    if (!isUp)
        handleMessageWhenDown(msg);
    else if (msg->isSelfMessage())
        handleSelfMessage(msg);
    else
        handlePacket(check_and_cast<Packet *>(msg));
}

void GlobalArp::handleSelfMessage(cMessage *msg)
{
    throw cRuntimeError("Model error: unexpected self message");
}

void GlobalArp::handlePacket(Packet *packet)
{
    EV << "Packet " << packet << " arrived, dropping it\n";
    delete packet;
}

void GlobalArp::handleMessageWhenDown(cMessage *msg)
{
    if (msg->isSelfMessage())
        throw cRuntimeError("Model error: self msg '%s' received when protocol is down", msg->getName());
    EV_ERROR << "Protocol is turned off, dropping '" << msg->getName() << "' message\n";
    delete msg;
}

bool GlobalArp::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if (static_cast<NodeStartOperation::Stage>(stage) == NodeStartOperation::STAGE_NETWORK_LAYER)
            start();
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if (static_cast<NodeShutdownOperation::Stage>(stage) == NodeShutdownOperation::STAGE_NETWORK_LAYER)
            stop();
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if (static_cast<NodeCrashOperation::Stage>(stage) == NodeCrashOperation::STAGE_CRASH)
            stop();
    }
    else
        throw cRuntimeError("Unsupported operation '%s'", operation->getClassName());
    return true;
}

void GlobalArp::start()
{
    isUp = true;
}

void GlobalArp::stop()
{
    isUp = false;
}

bool GlobalArp::isNodeUp()
{
    NodeStatus *nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
    return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
}

MacAddress GlobalArp::resolveL3Address(const L3Address& address, const InterfaceEntry *ie)
{
    Enter_Method_Silent();

    Ipv4Address addr = address.toIpv4();
    ArpCache::const_iterator it = globalArpCache.find(addr);
    if (it != globalArpCache.end())
        return it->second->macAddress;

    throw cRuntimeError("GlobalArp does not support dynamic address resolution");
    return MacAddress::UNSPECIFIED_ADDRESS;
}

L3Address GlobalArp::getL3AddressFor(const MacAddress& macAddr) const
{
    Enter_Method_Silent();

    if (macAddr.isUnspecified())
        return Ipv4Address::UNSPECIFIED_ADDRESS;

    for (ArpCache::const_iterator it = globalArpCache.begin(); it != globalArpCache.end(); it++)
        if (it->second->macAddress == macAddr && it->first.getType() == L3Address::IPv4)
            return it->first.toIpv4();


    return Ipv4Address::UNSPECIFIED_ADDRESS;
}

void GlobalArp::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    Enter_Method_Silent();
    // host associated. Link is up. Change the state to init.
    if (signalID == interfaceIpv4ConfigChangedSignal) {
        const InterfaceEntryChangeDetails *iecd = check_and_cast<const InterfaceEntryChangeDetails *>(obj);
        InterfaceEntry *ie = iecd->getInterfaceEntry();
        // rebuild the arp cache
        if (ie->isLoopback())
            return;
        auto it = globalArpCache.begin();
        for ( ; it != globalArpCache.end(); ++it) {
            if (it->second->interfaceEntry == ie)
                break;
        }
        ArpCacheEntry *entry = nullptr;
        if (it == globalArpCache.end()) {
            if (!ie->ipv4Data() || ie->ipv4Data()->getIPAddress().isUnspecified())
                return; // if the address is not defined it isn't included in the global cache
            entry = new ArpCacheEntry();
            entry->owner = this;
            entry->interfaceEntry = ie;
        }
        else {
            // actualize
            entry = it->second;
            ASSERT(entry->owner == this);
            globalArpCache.erase(it);
            if (!ie->ipv4Data() || ie->ipv4Data()->getIPAddress().isUnspecified()) {
                delete entry;
                return;    // if the address is not defined it isn't included in the global cache
            }
        }
        entry->macAddress = ie->getMacAddress();
        Ipv4Address ipAddr = ie->ipv4Data()->getIPAddress();
        auto where = globalArpCache.insert(globalArpCache.begin(), std::make_pair(ipAddr, entry));
        ASSERT(where->second == entry);
    }
}

} // namespace inet

