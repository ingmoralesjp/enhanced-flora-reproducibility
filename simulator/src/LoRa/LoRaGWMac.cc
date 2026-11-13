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

#include "LoRaGWMac.h"
#include "inet/common/ModuleAccess.h"
#include "../LoRaPhy/LoRaPhyPreamble_m.h"
#include "../LoRaPhy/LoRaAirtimeCalculator.h"
#include "inet/common/ProtocolTag_m.h"


#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"


namespace flora {

Define_Module(LoRaGWMac);

void LoRaGWMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        // subscribe for the information of the carrier sense
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        //radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);
        waitingForDC = false;
        dutyCycleTimer = new cMessage("Duty Cycle Timer");
        const char *addressString = par("address");
        GW_forwardedDown = 0;
        GW_droppedDC = 0;
        enforceDutyCycle = par("enforceDutyCycle").boolValue();
        gatewayDutyCycle = par("gatewayDutyCycle").doubleValue();
        airtimeModel = par("airtimeModel").stdstringValue();
        dutyCycleModel = par("dutyCycleModel").stdstringValue();
        if (gatewayDutyCycle <= 0.0 || gatewayDutyCycle > 1.0)
            throw cRuntimeError("gatewayDutyCycle must be in (0,1], got %g", gatewayDutyCycle);
        if (airtimeModel != "semtech" && airtimeModel != "legacyFixedPayload")
            throw cRuntimeError("airtimeModel must be semtech or legacyFixedPayload");
        if (dutyCycleModel != "toa" && dutyCycleModel != "legacySfConstants")
            throw cRuntimeError("dutyCycleModel must be toa or legacySfConstants");
        GW_downlinkTimeOnAir = 0.0;
        GW_dutyCycleHoldoff = 0.0;
        if (!strcmp(addressString, "auto")) {
            // assign automatic address
            address = MacAddress::generateAutoAddress();
            // change module parameter from "auto" to concrete address
            par("address").setStringValue(address.str().c_str());
        }
        else
            address.setAddress(addressString);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        radio->setRadioMode(IRadio::RADIO_MODE_TRANSCEIVER);
    }
}

void LoRaGWMac::finish()
{
    recordScalar("GW_forwardedDown", GW_forwardedDown);
    recordScalar("GW_droppedDC", GW_droppedDC);
    recordScalar("GW_configuredDutyCycle", gatewayDutyCycle);
    recordScalar("GW_airtimeModelCode", airtimeModel == "semtech" ? 1 : 0);
    recordScalar("GW_dutyCycleModelCode", dutyCycleModel == "toa" ? 1 : 0);
    recordScalar("GW_downlinkTimeOnAir", GW_downlinkTimeOnAir);
    recordScalar("GW_dutyCycleHoldoff", GW_dutyCycleHoldoff);
    cancelAndDelete(dutyCycleTimer);

}


void LoRaGWMac::configureNetworkInterface()
{
    //NetworkInterface *e = new NetworkInterface(this);

    // data rate
    //e->setDatarate(bitrate);

    // generate a link-layer address to be used as interface token for IPv6
    //e->setMACAddress(address);
    //e->setInterfaceToken(address.formInterfaceIdentifier());

    // capabilities
    //e->setMtu(par("mtu"));
    //e->setMulticast(true);
    //e->setBroadcast(true);
    //e->setPointToPoint(false);
    MacAddress address = parseMacAddressParameter(par("address"));

    // generate a link-layer address to be used as interface token for IPv6
    networkInterface->setMacAddress(address);
    // data rate
    //interfaceEntry->setDatarate(bitrate);

    // capabilities
    networkInterface->setMtu(par("mtu"));
    networkInterface->setMulticast(true);
    networkInterface->setBroadcast(true);
    networkInterface->setPointToPoint(false);
}

void LoRaGWMac::handleSelfMessage(cMessage *msg)
{
    if(msg == dutyCycleTimer) waitingForDC = false;
}

void LoRaGWMac::handleUpperMessage(cMessage *msg)
{
    if(waitingForDC == false)
    {
//        LoRaMacFrame *frame = check_and_cast<LoRaMacFrame *>(msg);
//        frame->removeControlInfo();
        auto pkt = check_and_cast<Packet *>(msg);
        const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
        if (pkt->getControlInfo())
            delete pkt->removeControlInfo();

        auto tag = pkt->addTagIfAbsent<MacAddressReq>();
        tag->setDestAddress(frame->getReceiverAddress());
//        LoRaMacControlInfo *ctrl = new LoRaMacControlInfo();
//        ctrl->setSrc(address);
//        ctrl->setDest(frame->getReceiverAddress());
//        frame->setControlInfo(ctrl);
//        sendDown(frame);


        // The complete MAC frame is the LoRa PHY payload at this point.  Use
        // the same Semtech-compatible calculator as the physical transmitter
        // instead of the former SF-only constants.  For duty-cycle fraction D,
        // two transmission starts must be separated by ToA/D; equivalently,
        // the post-transmission off-time is ToA*(1/D-1).
        const int payloadBytes = (pkt->getDataLength().get() + 7) / 8;
        const LoRaAirtime airtime = airtimeModel == "semtech" ?
                calculateLoRaAirtime(frame->getLoRaSF(), frame->getLoRaBW().get(),
                        payloadBytes, frame->getLoRaCR(), true) :
                calculateLegacyLoRaAirtime(frame->getLoRaSF(), frame->getLoRaBW().get(),
                        15, frame->getLoRaCR());
        const double timeOnAir = airtime.total().dbl();
        static const double legacyHoldoffSeconds[] = {
            0.61696, 1.23392, 2.14016, 4.28032, 7.24992, 14.49984
        };
        const int sfIndex = frame->getLoRaSF() - 7;
        if (sfIndex < 0 || sfIndex >= 6)
            throw cRuntimeError("Invalid LoRa spreading factor %d", frame->getLoRaSF());
        const double holdoff = !enforceDutyCycle ? timeOnAir :
                dutyCycleModel == "toa" ? timeOnAir / gatewayDutyCycle :
                legacyHoldoffSeconds[sfIndex];

        waitingForDC = true;
        scheduleAt(simTime() + holdoff, dutyCycleTimer);
        GW_downlinkTimeOnAir += timeOnAir;
        GW_dutyCycleHoldoff += holdoff;
        GW_forwardedDown++;
        pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);
        sendDown(pkt);
    }
    else
    {
        GW_droppedDC++;
        delete msg;
    }
}

void LoRaGWMac::handleLowerMessage(cMessage *msg)//---------------/////////////-----------------------------------------
{
    auto pkt = check_and_cast<Packet *>(msg);
    auto header = pkt->popAtFront<LoRaPhyPreamble>();
    const auto &frame = pkt->peekAtFront<LoRaMacFrame>();

    if(frame->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS)
    {
                sendUp(pkt);
    }

    else
        delete pkt;
}

void LoRaGWMac::sendPacketBack(Packet *receivedFrame)
{
    const auto &frame = receivedFrame->peekAtFront<LoRaMacFrame>();
    EV << "sending Data frame back" << endl;
    auto pktBack = new Packet("LoraPacket");
    auto frameToSend = makeShared<LoRaMacFrame>();
    frameToSend->setChunkLength(B(par("headerLength").intValue()));

    frameToSend->setReceiverAddress(frame->getTransmitterAddress());
    pktBack->insertAtFront(frameToSend);
    sendDown(pktBack);
}

void LoRaGWMac::createFakeLoRaMacFrame()
{

}

void LoRaGWMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            //transmissin is finished
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        }
        transmissionState = newRadioTransmissionState;
    }
}

MacAddress LoRaGWMac::getAddress()
{
    return address;
}

}
