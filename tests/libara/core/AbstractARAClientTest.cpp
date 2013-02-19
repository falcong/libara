/******************************************************************************
 Copyright 2012, The DES-SERT Team, Freie Universität Berlin (FUB).
 All rights reserved.

 These sources were originally developed by Friedrich Große
 at Freie Universität Berlin (http://www.fu-berlin.de/),
 Computer Systems and Telematics / Distributed, Embedded Systems (DES) group
 (http://cst.mi.fu-berlin.de/, http://www.des-testbed.net/)
 ------------------------------------------------------------------------------
 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation, either version 3 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with
 this program. If not, see http://www.gnu.org/licenses/ .
 ------------------------------------------------------------------------------
 For further information and questions please use the web site
 http://www.des-testbed.net/
 *******************************************************************************/

#include "CppUTest/TestHarness.h"
#include "PacketTrap.h"
#include "RoutingTable.h"
#include "Packet.h"
#include "PacketType.h"
#include "Address.h"
#include "Pair.h"

#include "testAPI/mocks/ARAClientMock.h"
#include "testAPI/mocks/PacketMock.h"
#include "testAPI/mocks/NetworkInterfaceMock.h"
#include "testAPI/mocks/AddressMock.h"

using namespace ARA;

typedef std::shared_ptr<Address> AddressPtr;

TEST_GROUP(AbstractARAClientTest) {
    ARAClientMock* client;
    PacketTrap* packetTrap;
    RoutingTable* routingTable;

    void setup() {
        client = new ARAClientMock();
        packetTrap = client->getPacketTrap();
        routingTable = client->getRoutingTable();
    }
    void teardown() {
        delete client;
    }

    /**
     * Returns true iff a route to destination via nextHop and interface is known in the RoutingTable.
     */
    bool aRouteToDestinationIsKnown(AddressPtr destination, AddressPtr nextHop, NetworkInterface* interface) {
        std::deque<RoutingTableEntry*>* possibleNextHops = routingTable->getPossibleNextHops(destination);
        for(auto& possibleHop: *possibleNextHops) {
            if(possibleHop->getAddress()->equals(nextHop) && possibleHop->getNetworkInterface()->equals(interface)) {
                return true;
            }
        }

        return false;
    }
};

TEST(AbstractARAClientTest, packetGetsTrappedIfNotDeliverable) {
    PacketMock packet = PacketMock();

    CHECK(routingTable->isDeliverable(&packet) == false);
    client->sendPacket(&packet);
    CHECK(packetTrap->contains(&packet));
}

TEST(AbstractARAClientTest, generalBroadCast) {
    NetworkInterfaceMock* interface1 = client->createNewNetworkInterfaceMock();
    NetworkInterfaceMock* interface2 = client->createNewNetworkInterfaceMock();
    NetworkInterfaceMock* interface3 = client->createNewNetworkInterfaceMock();

    PacketMock packet = PacketMock();
    client->broadCast(&packet);

    CHECK(interface1->hasPacketBeenBroadCasted(&packet) == true);
    CHECK(interface2->hasPacketBeenBroadCasted(&packet) == true);
    CHECK(interface3->hasPacketBeenBroadCasted(&packet) == true);
}

TEST(AbstractARAClientTest, getNextSequenceNumber) {
    for (unsigned int i = 1; i < 100; i++) {
        CHECK_EQUAL(i, client->getNextSequenceNumber());
    }
}

TEST(AbstractARAClientTest, broadcastFANTIfPacketNotDeliverable) {
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock();
    PacketMock packet = PacketMock();

    CHECK(routingTable->isDeliverable(&packet) == false);
    client->sendPacket(&packet);

    LONGS_EQUAL(1, interface->getNumberOfSentPackets());
    Pair<Packet*, AddressPtr>* sentPacketInfo = interface->getSentPackets()->front();
    Packet* sentPacket = sentPacketInfo->getLeft();
    AddressPtr recipientOfSentPacket = sentPacketInfo->getRight();

    CHECK(sentPacket->getType() == PacketType::FANT);
    CHECK(interface->isBroadcastAddress(recipientOfSentPacket));
}

TEST(AbstractARAClientTest, sendPacketToNextHopIfRouteIsKnown) {
    NetworkInterfaceMock* interface1 = client->createNewNetworkInterfaceMock();
    NetworkInterfaceMock* interface2 = client->createNewNetworkInterfaceMock();
    NetworkInterfaceMock* interface3 = client->createNewNetworkInterfaceMock();
    PacketMock originalPacket = PacketMock();
    AddressPtr nextHop (new AddressMock("nextHop"));

    // make sure that a route to the packet destination is available
    routingTable->update(originalPacket.getDestination(), nextHop, interface2, 1.0);
    CHECK(routingTable->isDeliverable(&originalPacket) == true);

    client->sendPacket(&originalPacket);

    // check if packet has been send over the correct interface
    CHECK(interface1->hasPacketBeenSend(&originalPacket) == false);
    CHECK(interface2->hasPacketBeenSend(&originalPacket) == true);
    CHECK(interface3->hasPacketBeenSend(&originalPacket) == false);

    // check if packet has been send via interface2 to nextHop
    Pair<Packet*, AddressPtr>* sentPacketInfo = interface2->getSentPackets()->front();
    Packet* sentPacket = sentPacketInfo->getLeft();
    AddressPtr recipientOfSentPacket = sentPacketInfo->getRight();
    CHECK(recipientOfSentPacket->equals(nextHop));

    // Check that packet content is basically the same
    CHECK_EQUAL(originalPacket.getType(), sentPacket->getType());
    CHECK(sentPacket->getSource()->equals(originalPacket.getSource()));
    CHECK(sentPacket->getDestination()->equals(originalPacket.getDestination()));
    CHECK_EQUAL(originalPacket.getSequenceNumber(), sentPacket->getSequenceNumber());
    CHECK_EQUAL(originalPacket.getPayload(), sentPacket->getPayload());
    CHECK_EQUAL(originalPacket.getPayloadLength(), sentPacket->getPayloadLength());

    // only the hop count needs to be incremented by 1
    CHECK_EQUAL(originalPacket.getHopCount() + 1, sentPacket->getHopCount());
}

TEST(AbstractARAClientTest, getNumberOfNetworkInterfaces) {
    NetworkInterfaceMock interface1 = NetworkInterfaceMock();
    NetworkInterfaceMock interface2 = NetworkInterfaceMock();
    NetworkInterfaceMock interface3 = NetworkInterfaceMock();
    CHECK_EQUAL(0, client->getNumberOfNetworkInterfaces());

    client->addNetworkInterface(&interface1);
    CHECK_EQUAL(1, client->getNumberOfNetworkInterfaces());

    client->addNetworkInterface(&interface2);
    CHECK_EQUAL(2, client->getNumberOfNetworkInterfaces());

    client->addNetworkInterface(&interface3);
    CHECK_EQUAL(3, client->getNumberOfNetworkInterfaces());
}

TEST(AbstractARAClientTest, getNetworkInterface) {
    NetworkInterfaceMock interface1 = NetworkInterfaceMock();
    NetworkInterfaceMock interface2 = NetworkInterfaceMock();
    NetworkInterfaceMock interface3 = NetworkInterfaceMock();
    client->addNetworkInterface(&interface1);
    client->addNetworkInterface(&interface2);
    client->addNetworkInterface(&interface3);

    CHECK(client->getNetworkInterface(0) == &interface1);
    CHECK(client->getNetworkInterface(1) == &interface2);
    CHECK(client->getNetworkInterface(2) == &interface3);
}

TEST(AbstractARAClientTest, registerReceivedPacket) {
    PacketMock packet1 = PacketMock("A", "B", 123);
    PacketMock packet2 = PacketMock("A", "B", 124);  // same source but different seqNr
    PacketMock packet3 = PacketMock("C", "A", 123);  // same seqNr but different source
    PacketMock packet4 = PacketMock("A", "C", 123);  // the source and seqNr are equal to packet1

    CHECK_FALSE(client->hasBeenReceivedEarlier(&packet1));
    CHECK_FALSE(client->hasBeenReceivedEarlier(&packet2));
    CHECK_FALSE(client->hasBeenReceivedEarlier(&packet3));
    CHECK_FALSE(client->hasBeenReceivedEarlier(&packet4));

    client->registerReceivedPacket(&packet1);
    CHECK_TRUE(client->hasBeenReceivedEarlier(&packet1));
    CHECK_FALSE(client->hasBeenReceivedEarlier(&packet2));
    CHECK_FALSE(client->hasBeenReceivedEarlier(&packet3));
    CHECK_TRUE(client->hasBeenReceivedEarlier(&packet4));
}

/**
 * In this test we simulate that the same data packet has been received
 * twice at node x. The packet is directed from node A to node B and
 * has been relayed via node C to node X.
 * Node x must respond to node C with a DUPLICATE_ERROR packet.
 */
TEST(AbstractARAClientTest, respondWithDuplicateError) {
    // prepare a packet
    PacketMock packet = PacketMock("A", "B", "C", 123, 1, PacketType::DATA);

    // let client receive the packet over the same interface twice
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock("X");
    client->receivePacket(&packet, interface);
    client->receivePacket(&packet, interface);

    // the client should have relayed the first packet and sent a duplicate warning back for the second packet
    LONGS_EQUAL(2, interface->getNumberOfSentPackets());

    Pair<Packet*, AddressPtr>* sentPacketInfo = interface->getSentPackets()->at(1); // we only check the warning
    Packet* sentPacket = sentPacketInfo->getLeft();
    AddressPtr recipientOfSentPacket = sentPacketInfo->getRight();

    // check the contents of the duplicate warning packet
    CHECK(recipientOfSentPacket->equals(packet.getSender()));
    CHECK(sentPacket->getSender()->equals(interface->getLocalAddress()));
    CHECK(sentPacket->getSource()->equals(packet.getSource()));
    CHECK(sentPacket->getType() == PacketType::DUPLICATE_ERROR);
    LONGS_EQUAL(packet.getHopCount()+1, sentPacket->getHopCount());
    CHECK_EQUAL(0, sentPacket->getPayloadLength());
}

/**
 * In this test we simulate that a FANT or a BANT have been received
 * twice at node x. This kind of duplication must simply be ignored
 * by node x (no further broadcasting of the ant packet).
 */
TEST(AbstractARAClientTest, ignoreDuplicateAntPackets) {
    // prepare the ant packets
    PacketMock fant = PacketMock("A", "B", "C", 123, 1, PacketType::FANT);
    PacketMock bant = PacketMock("B", "A", "C", 456, 4, PacketType::BANT);

    // let client receive the packets over the same interface twice or more
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock("X");
    client->receivePacket(&fant, interface);
    client->receivePacket(&fant, interface);
    client->receivePacket(&bant, interface);
    client->receivePacket(&bant, interface);
    client->receivePacket(&bant, interface);

    LONGS_EQUAL(2, interface->getNumberOfSentPackets()); // only two broadcasts of the ANT packets and nothing more
    std::deque<Pair<Packet*, AddressPtr>*>* sendPackets = interface->getSentPackets();
    CHECK_EQUAL(PacketType::FANT, sendPackets->at(0)->getLeft()->getType());
    CHECK_EQUAL(PacketType::BANT, sendPackets->at(1)->getLeft()->getType());
}

/**
 * This test checks whether the routing table is updated if a new packet has been received.
 * The packet will be send from node A to node B via node C and the current node D.
 * After the packet has been processed at node D a route to node A (back via node C) must be
 * known to node D.
 *
 * (A)-->(C)-->(D)-->(B)
 *
 * Note that we do not check how the table is updated at this point because this depends
 * on the concrete implementation of the AbstractARAclient->
 */
TEST(AbstractARAClientTest, routingTableIsUpdated) {
    NetworkInterface* interface = client->createNewNetworkInterfaceMock();
    AddressPtr nodeA (new AddressMock("A"));
    AddressPtr nodeB (new AddressMock("B"));
    AddressPtr nodeC (new AddressMock("C"));
    Packet packet = Packet(nodeA, nodeB, nodeC, PacketType::DATA, 123);

    CHECK(routingTable->isDeliverable(nodeA) == false);
    client->receivePacket(&packet, interface);
    CHECK(routingTable->isDeliverable(nodeA) == true);
    std::deque<RoutingTableEntry*>* possibleHops = routingTable->getPossibleNextHops(nodeA);
    LONGS_EQUAL(1, possibleHops->size());
    CHECK(possibleHops->front()->getAddress()->equals(nodeC));
    CHECK(possibleHops->front()->getNetworkInterface()->equals(interface));
}

/**
 * This test checks if a data packet that is received at the current node
 * (with address 192.168.0.1) is delivered to the system if it is directed
 * to the current node.
 */
TEST(AbstractARAClientTest, dataPacketIsDeliveredToSystem) {
    std::deque<const Packet*>* deliveredPackets = client->getDeliveredPackets();
    NetworkInterface* interface = client->createNewNetworkInterfaceMock("192.168.0.1");
    PacketMock packet1 = PacketMock("192.168.0.4", "192.168.0.123", 123, 1, PacketType::DATA);// directed to some other node
    PacketMock packet2 = PacketMock("192.168.0.4", "192.168.0.1",   124);  // directed to this node

    CHECK(deliveredPackets->empty())

    client->receivePacket(&packet1, interface);
    CHECK(deliveredPackets->empty())

    client->receivePacket(&packet2, interface);
    LONGS_EQUAL(1, deliveredPackets->size());
    CHECK(deliveredPackets->front()->equals(&packet2));
}

/**
 * In this test a node A receives a data packet from node B which is
 * directed to another node C. The route to that node is known.
 * It is expected that the packet is relayed through the known route.
 */
TEST(AbstractARAClientTest, dataPacketIsRelayedIfRouteIsKnown) {
    // initial test setup
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock("A");
    std::deque<Pair<Packet*, AddressPtr>*>* sentPackets = interface->getSentPackets();

    AddressPtr source (new AddressMock("B"));
    AddressPtr destination (new AddressMock("C"));
    AddressPtr sender = source;
    Packet packet = Packet(source, destination, sender, PacketType::DATA, 123, "Hello World");
    packet.increaseHopCount();

    // create a route to the destination
    routingTable->update(destination, destination, interface, 10);

    // start the test
    client->receivePacket(&packet, interface);
    CHECK(sentPackets->size() == 1);
    Pair<Packet*, AddressPtr>* sentPacketInfo = sentPackets->front();
    CHECK(sentPacketInfo->getRight()->equals(destination)); // packet has been sent to destination

    // check the sent packet
    Packet* sentPacket = sentPacketInfo->getLeft();
    CHECK(sentPacket->getSource()->equals(source));
    CHECK(sentPacket->getDestination()->equals(destination));
    CHECK(sentPacket->getSender()->equals(interface->getLocalAddress()));
    CHECK_EQUAL(PacketType::DATA, sentPacket->getType());
    STRCMP_EQUAL("Hello World", sentPacket->getPayload());
    LONGS_EQUAL(123, sentPacket->getSequenceNumber());
    LONGS_EQUAL(2, sentPacket->getHopCount());
}

/**
 * In this test node A receives a FANT from node B which is directed to
 * node C. The FANT is expected to be broadcast over all interfaces.
 * Node A does also receive a BANT back from C directed to node B which
 * is also expected to be broadcast.
 */
TEST(AbstractARAClientTest, receivedAntPacketsAreBroadcasted) {
    // initial test setup
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock("A");
    std::deque<Pair<Packet*, AddressPtr>*>* sentPackets = interface->getSentPackets();

    PacketMock packet1 = PacketMock("B", "C", 123, 1, PacketType::FANT);
    PacketMock packet2 = PacketMock("C", "B", 345, 1, PacketType::BANT);

    // start the test
    client->receivePacket(&packet1, interface);
    client->receivePacket(&packet2, interface);

    CHECK(sentPackets->size() == 2);
    Pair<Packet*, AddressPtr>* sentPacketInfo1 = sentPackets->at(0);
    Pair<Packet*, AddressPtr>* sentPacketInfo2 = sentPackets->at(1);

    // check the first sent packet
    CHECK(interface->isBroadcastAddress(sentPacketInfo1->getRight()));
    Packet* sentPacket1 = sentPacketInfo1->getLeft();
    CHECK(sentPacket1->getSource()->equals(packet1.getSource()));
    CHECK(sentPacket1->getDestination()->equals(packet1.getDestination()));
    CHECK(sentPacket1->getSender()->equals(interface->getLocalAddress()));
    CHECK_EQUAL(PacketType::FANT, sentPacket1->getType());
    LONGS_EQUAL(2, sentPacket1->getHopCount());

    // check the first sent packet
    Packet* sentPacket2 = sentPacketInfo2->getLeft();
    CHECK(sentPacket2->getSource()->equals(packet2.getSource()));
    CHECK(sentPacket2->getDestination()->equals(packet2.getDestination()));
    CHECK(sentPacket2->getSender()->equals(interface->getLocalAddress()));
    CHECK_EQUAL(PacketType::BANT, sentPacket2->getType());
    LONGS_EQUAL(2, sentPacket2->getHopCount());
}

/**
 * In this test node A receives a FANT directed from node B via node C
 * to node A itself. It is expected to broadcast a new BANT directed to
 * the FANts source.
 *
 * (B)--FANT->(C)--FANT->(A)
 *  └-<-BANT--(C)<-BANT---┘
 */
TEST(AbstractARAClientTest, receivedFANTTriggersNewBANT) {
    // initial test setup
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock("A");
    std::deque<Pair<Packet*, AddressPtr>*>* sentPackets = interface->getSentPackets();
    unsigned int lastSequenceNumber = client->getNextSequenceNumber();

    AddressPtr nodeB (new AddressMock("B"));
    AddressPtr nodeA (new AddressMock("A"));
    AddressPtr nodeC (new AddressMock("C"));
    Packet packet = Packet(nodeB, nodeA, nodeC, PacketType::FANT, 123);

    // start the test
    client->receivePacket(&packet, interface);

    // check the sent packet
    CHECK(sentPackets->size() == 1);
    Pair<Packet*, AddressPtr>* sentPacketInfo = sentPackets->front();
    CHECK(interface->isBroadcastAddress(sentPacketInfo->getRight()));
    Packet* sentPacket = sentPacketInfo->getLeft();
    CHECK(sentPacket->getSource()->equals(nodeA));
    CHECK(sentPacket->getDestination()->equals(nodeB));
    CHECK(sentPacket->getSender()->equals(nodeA));
    CHECK_EQUAL(PacketType::BANT, sentPacket->getType());
    LONGS_EQUAL(1, sentPacket->getHopCount());
    LONGS_EQUAL(lastSequenceNumber+1, sentPacket->getSequenceNumber());
}

/**
 * In this test node A has initiated a route discovery for a trapped data packet
 * Now it has received a BANT that has come back from the data packets destination.
 * It is expected that the trapped packet is send over the discovered route and is
 * no longer trapped in the PacketTrap.
 *
 */
TEST(AbstractARAClientTest, receivedBANTTriggersSendingOfTrappedPackets) {
    // initial test setup
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock("A");
    std::deque<Pair<Packet*, AddressPtr>*>* sentPackets = interface->getSentPackets();

    PacketMock dataPacket = PacketMock("A", "C");
    packetTrap->trapPacket(&dataPacket);

    PacketMock bant = PacketMock("C", "A", "B", 123, 2, PacketType::BANT);

    // start the test
    client->receivePacket(&bant, interface);

    // check the sent packet
    LONGS_EQUAL(1, sentPackets->size());
    Pair<Packet*, AddressPtr>* sentPacketInfo = sentPackets->front();
    CHECK(sentPacketInfo->getRight()->equals(bant.getSender()));
    Packet* sentPacket = sentPacketInfo->getLeft();
    CHECK(sentPacket->getSource()->equals(dataPacket.getSource()));
    CHECK(sentPacket->getDestination()->equals(dataPacket.getDestination()));
    CHECK(sentPacket->getSender()->equals(interface->getLocalAddress()));
    CHECK_EQUAL(PacketType::DATA, sentPacket->getType());
    LONGS_EQUAL(1, sentPacket->getHopCount());

    // packet trap must now be empty
    CHECK(packetTrap->isEmpty());
}

/**
 * In this test we check if a client broadcasts a FANT again that he has
 * initially created and sent in the first place.
 */
TEST(AbstractARAClientTest, doNotReBroadcastFANT) {
    // initial test setup
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock("A");
    std::deque<Pair<Packet*, AddressPtr>*>* sentPackets = interface->getSentPackets();
    PacketMock data = PacketMock("A", "Z", "A", 123, 1, PacketType::DATA);

    // sending the initial packet should trigger a FANT broadcast
    client->sendPacket(&data);
    CHECK(sentPackets->size() == 1);
    Pair<Packet*, AddressPtr>* sentPacketInfo = sentPackets->at(0);
    Packet* sentPacket = sentPacketInfo->getLeft();
    CHECK(sentPacket->getType() == PacketType::FANT);
    sentPackets->clear();
    delete sentPacketInfo;

    // emulate that the neighbor does also broadcast the FANT and this client receives it
    AddressPtr neighborAddress (new AddressMock("B"));
    Packet* answer = sentPacket->clone();
    answer->increaseHopCount();
    answer->setSender(neighborAddress);
    client->receivePacket(answer, interface);

    // the client should not broadcast the FANT again
    CHECK(sentPackets->empty())

    // cleanup
    delete answer;
    delete sentPacket;
}

/**
 *
 */
TEST(AbstractARAClientTest, doNotReBroadcastBANT) {
    // initial test setup
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock("Z");
    std::deque<Pair<Packet*, AddressPtr>*>* sentPackets = interface->getSentPackets();
    PacketMock fant = PacketMock("A", "Z", "A", 123, 1, PacketType::FANT);

    // client receives the FANT that is directed to him (should trigger BANt broadcast)
    client->receivePacket(&fant, interface);
    CHECK(sentPackets->size() == 1);
    Pair<Packet*, AddressPtr>* sentPacketInfo = sentPackets->at(0);
    Packet* sentPacket = sentPacketInfo->getLeft();
    CHECK(sentPacket->getType() == PacketType::BANT);
    sentPackets->clear();
    delete sentPacketInfo;

    // emulate that the neighbor does also broadcast the BANT and this client receives it back
    AddressPtr neighborAddress (new AddressMock("Y"));
    Packet* answer = sentPacket->clone();
    answer->increaseHopCount();
    answer->setSender(neighborAddress);
    client->receivePacket(answer, interface);

    // the client should not broadcast the BANT again
    CHECK(sentPackets->empty())

    // cleanup
    delete answer;
    delete sentPacket;
}

/**
 * In this test we check if a client deletes the routing table entry to
 * another note if he receives a duplicate warning from this node.
 *
 * Test setup
 * Before test:        After test:
 *
 *    /--> (B)            /--> (B)
 *  (A)--> (C)          (A)
 *    \--> (D)            \--> (D)
 */
TEST(AbstractARAClientTest, receiveDuplicateErrorPacket) {
    // initial test setup
    NetworkInterfaceMock* interface = client->createNewNetworkInterfaceMock("A");

    PacketMock duplicateErrorPacket = PacketMock("A", "X", "C", 123, 1, PacketType::DUPLICATE_ERROR);

    AddressPtr nodeB (new AddressMock("B"));
    AddressPtr nodeC (new AddressMock("C"));
    AddressPtr nodeD (new AddressMock("D"));
    AddressPtr destination (new AddressMock("X"));
    routingTable->update(destination, nodeB, interface, 1.1);
    routingTable->update(destination, nodeC, interface, 2.2);
    routingTable->update(destination, nodeD, interface, 3.3);

    // Check some basic assumptions about this test setup
    CHECK(aRouteToDestinationIsKnown(destination, nodeC, interface));

    // start the test
    client->receivePacket(&duplicateErrorPacket, interface);
    CHECK(aRouteToDestinationIsKnown(destination, nodeC, interface) == false);
}