//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "OMNeTARAClient.h"
#include <omnetpp.h>
#include "OMNeTPacket.h"
#include "OMNeTAddress.h"
#include "OMNeTGate.h"

using namespace ARA;

// The module class needs to be registered with OMNeT++
Define_Module(OMNeTARAClient);

void OMNeTARAClient::initialize() {
    for (cModule::GateIterator i(this); !i.end(); i++) {
        cGate* gate = i();
        if(gate->getType() == cGate::OUTPUT) {
            addNetworkInterface(new OMNeTGate(this, gate));
        }
    }

    if (strcmp("source", getName()) == 0) {
        sendInitialPacket();
    }
}

void OMNeTARAClient::sendInitialPacket() {
    OMNeTAddress* source = new OMNeTAddress("source");
    OMNeTAddress* destination = new OMNeTAddress("destination");
    OMNeTPacket* msg = new OMNeTPacket(source, destination, source, PacketType::DATA, getNextSequenceNumber(), "Hello ARA World");
    sendPacket(msg);
}

void OMNeTARAClient::handleMessage(cMessage *msg) {
    OMNeTPacket* omnetPacket = (OMNeTPacket*) msg;
    //send(msg, "g$o", 0);
    receivePacket(omnetPacket, getNetworkInterface(0));
}

NextHop* OMNeTARAClient::getNextHop(const Packet* packet) {
    if(routingTable.isDeliverable(packet)) {
        LinkedList<RoutingTableEntry>* possibleHops = routingTable.getPossibleNextHops(packet);
        // search for the best value
        // TODO this can be replaced as soon as Michael is ready with the corresponding class
        RoutingTableEntry* bestHop = NULL;

        unsigned int nrOfPossibleRoutes = possibleHops->size();
        for (unsigned int i = 0; i < nrOfPossibleRoutes; ++i) {
            RoutingTableEntry* currentHop = possibleHops->get(i);
            if(bestHop == NULL || currentHop->getPheromoneValue() > bestHop->getPheromoneValue()) {
                bestHop = currentHop;
            }
        }

        return bestHop->getNextHop();
    }
    else {
        // TODO maybe it would be better to return a NULL Object (Pattern) instead of returning a NULL pointer
        return NULL;
    }
}

void OMNeTARAClient::updateRoutingTable(const Packet* packet, NetworkInterface* interface) {
    // TODO implement OMNeTARAClient::updateRoutingTable
}

void OMNeTARAClient::deliverToSystem(const Packet* packet) {
    EV << getName() << " delivered a packet to the system\n";
}
