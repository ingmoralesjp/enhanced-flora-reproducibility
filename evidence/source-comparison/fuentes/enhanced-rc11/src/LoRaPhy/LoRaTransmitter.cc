// Copyright (C) 2026
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LoRaTransmitter.h"

#include "inet/common/packet/Packet.h"

#include "LoRaModulation.h"
#include "LoRaPhyPreamble_m.h"

namespace flora {

using namespace inet;
using namespace inet::physicallayer;
using namespace omnetpp;

Define_Module(LoRaTransmitter);

void LoRaTransmitter::initialize(int stage)
{
    // LoRa uses its own LoRaTransmission representation, so only initialize
    // TransmitterBase here; FlatTransmitterBase would try to resolve the
    // string "LoRaModulation" through INET's generic modulation registry.
    TransmitterBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        headerLength = b(par("headerLength"));
        bitrate = bps(par("bitrate"));
        power = W(par("power"));
        centerFrequency = Hz(par("centerFrequency"));
        bandwidth = Hz(par("bandwidth"));

        isGateway = !strcmp(getParentModule()->getClassName(), "flora::LoRaGWRadio");
        airtimeModel = par("airtimeModel").stdstringValue();
        if (airtimeModel != "semtech" && airtimeModel != "legacyFixedPayload")
            throw cRuntimeError("airtimeModel must be semtech or legacyFixedPayload");
        referencePayloadSize = B(par("referencePayloadSize")).get();
        referenceCodeRedundance = par("referenceCodeRedundance");
        referenceUseHeader = par("referenceUseHeader");
        referenceBandwidth = Hz(par("referenceBandwidth"));

        transmissionCreatedSignal = registerSignal("LoRaTransmissionCreated");
        timeOnAirSignal = registerSignal("timeOnAir");
    }
}

const ITransmission *LoRaTransmitter::createTransmission(const IRadio *transmitter,
        const Packet *macFrame, const simtime_t startTime) const
{
    const auto& preamble = macFrame->peekAtFront<LoRaPhyPreamble>();

    // The PHY payload is the packet actually put on air: MAC header plus application
    // data. The simulator-only LoRaPhyPreamble chunk is removed from this byte count.
    const b phyPayloadBits = macFrame->getDataLength() - preamble->getChunkLength();
    const int payloadBytes = static_cast<int>((b(phyPayloadBits).get() + 7) / 8);
    const LoRaAirtime airtime = airtimeModel == "semtech" ?
            calculateLoRaAirtime(preamble->getSpreadFactor(),
                    preamble->getBandwidth().get(), payloadBytes,
                    preamble->getCodeRendundance(), preamble->getUseHeader()) :
            calculateLegacyLoRaAirtime(preamble->getSpreadFactor(),
                    preamble->getBandwidth().get(), isGateway ? 15 : 20,
                    preamble->getCodeRendundance());

    const simtime_t endTime = startTime + airtime.total();
    IMobility *mobility = transmitter->getAntenna()->getMobility();
    const Coord position = mobility->getCurrentPosition();
    const Quaternion orientation = mobility->getCurrentAngularPosition();
    const W transmissionPower = computeTransmissionPower(macFrame);

    // The energy consumer converts this radiated power to the corresponding
    // supply current from energyConsumptionParameters.xml.
    if (!isGateway)
        check_and_cast<LoRaRadio *>(getParentModule())->setCurrentTxPower(transmissionPower.get());

    transmissionCount++;
    accumulatedTimeOnAir += airtime.total();
    auto self = const_cast<LoRaTransmitter *>(this);
    self->emit(transmissionCreatedSignal, true);
    self->emit(timeOnAirSignal, airtime.total());

    return new LoRaTransmission(transmitter, macFrame, startTime, endTime,
            airtime.preamble, airtime.header, airtime.payload,
            position, position, orientation, orientation, transmissionPower,
            preamble->getCenterFrequency(), preamble->getSpreadFactor(),
            preamble->getBandwidth(), preamble->getCodeRendundance());
}

std::ostream& LoRaTransmitter::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "LoRaTransmitter";
    return FlatTransmitterBase::printToStream(stream, level, evFlags);
}

void LoRaTransmitter::finish()
{
    recordScalar("airtimeModelCode", airtimeModel == "semtech" ? 1 : 0);
    recordScalar("transmissionCount", transmissionCount);
    recordScalar("totalTimeOnAir", accumulatedTimeOnAir.dbl(), "s");
    recordScalar("meanTimeOnAir",
            transmissionCount == 0 ? 0 : accumulatedTimeOnAir.dbl() / transmissionCount, "s");

    // Reference scalars make formula regressions visible without transmitting six packets.
    // Their payload/CR/header assumptions are explicit NED parameters.
    for (int sf = 7; sf <= 12; sf++) {
        const LoRaAirtime reference = airtimeModel == "semtech" ?
                calculateLoRaAirtime(sf, referenceBandwidth.get(), referencePayloadSize,
                        referenceCodeRedundance, referenceUseHeader) :
                calculateLegacyLoRaAirtime(sf, referenceBandwidth.get(), isGateway ? 15 : 20,
                        referenceCodeRedundance);
        std::string name = "referenceToA_SF" + std::to_string(sf);
        recordScalar(name.c_str(), reference.total().dbl(), "s");
    }
}

} // namespace flora
