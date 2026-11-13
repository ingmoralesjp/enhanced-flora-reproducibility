#ifndef __FLORA_PACKETFORWARDER_H
#define __FLORA_PACKETFORWARDER_H

#include <vector>

#include <omnetpp.h>

#include "inet/common/INETDefs.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include "LoRaMacFrame_m.h"

namespace flora {

using namespace inet;
using namespace omnetpp;

/** Bridges LoRa MAC frames between one gateway radio and network servers. */
class PacketForwarder : public cSimpleModule
{
  protected:
    std::vector<L3Address> destinationAddresses;
    UdpSocket socket;
    int localPort = -1;
    int destinationPort = -1;
    long forwardedUplinks = 0;
    long forwardedDownlinks = 0;
    simsignal_t gatewayPacketReceivedSignal;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;
    virtual void finish() override;

    void startUdp();
    void forwardUplink(Packet *packet);
};

} // namespace flora

#endif
