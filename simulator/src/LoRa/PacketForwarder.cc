#include "PacketForwarder.h"

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

namespace flora {

Define_Module(PacketForwarder);

void PacketForwarder::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        gatewayPacketReceivedSignal = registerSignal("LoRa_GWPacketReceived");
        localPort = par("localPort");
        destinationPort = par("destPort");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
        startUdp();
}

void PacketForwarder::startUdp()
{
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);

    cStringTokenizer destinations(par("destAddresses").stringValue());
    while (destinations.hasMoreTokens()) {
        const char *name = destinations.nextToken();
        L3Address address;
        if (!L3AddressResolver().tryResolve(name, address) || address.isUnspecified())
            throw cRuntimeError("Cannot resolve network-server address '%s'", name);
        destinationAddresses.push_back(address);
    }
    if (destinationAddresses.empty())
        throw cRuntimeError("At least one network-server destination is required");
}

void PacketForwarder::handleMessage(cMessage *message)
{
    auto packet = check_and_cast<Packet *>(message);
    if (message->arrivedOn("lowerLayerIn")) {
        const auto& frame = packet->peekAtFront<LoRaMacFrame>();
        if (frame->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS)
            forwardUplink(packet);
        else
            delete packet;
    }
    else if (message->arrivedOn("socketIn")) {
        packet->peekAtFront<LoRaMacFrame>(); // validates the downlink format
        forwardedDownlinks++;
        send(packet, "lowerLayerOut");
    }
    else
        throw cRuntimeError("Unexpected packet on gate '%s'", message->getArrivalGate()->getName());
}

void PacketForwarder::forwardUplink(Packet *packet)
{
    packet->trimFront();
    auto frame = packet->removeAtFront<LoRaMacFrame>();
    const auto& snir = packet->getTag<SnirInd>();
    const auto& signalPower = packet->getTag<SignalPowerInd>();

    frame->setRSSI(math::mW2dBmW(signalPower->getPower().get() * 1000));
    frame->setSNIR(snir->getMinimumSnir());
    packet->insertAtFront(frame);

    if (packet->getControlInfo() != nullptr)
        delete packet->removeControlInfo();
    emit(gatewayPacketReceivedSignal, 1L);
    forwardedUplinks++;
    socket.sendTo(packet, destinationAddresses.front(), destinationPort);
}

void PacketForwarder::finish()
{
    recordScalar("forwardedUplinks", forwardedUplinks);
    recordScalar("forwardedDownlinks", forwardedDownlinks);
}

} // namespace flora
