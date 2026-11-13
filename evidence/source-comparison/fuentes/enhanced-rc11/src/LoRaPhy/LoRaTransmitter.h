// Copyright (C) 2026
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef LORAPHY_LORATRANSMITTER_H_
#define LORAPHY_LORATRANSMITTER_H_

#include <string>

#include "inet/physicallayer/wireless/common/base/packetlevel/FlatTransmitterBase.h"

#include "LoRa/LoRaRadio.h"
#include "LoRaAirtimeCalculator.h"
#include "LoRaTransmission.h"

namespace flora {

/** LoRa transmitter with Semtech-compatible time-on-air calculation. */
class LoRaTransmitter : public inet::physicallayer::FlatTransmitterBase
{
  protected:
    bool isGateway = false;
    std::string airtimeModel = "semtech";
    int referencePayloadSize = 24;
    int referenceCodeRedundance = 1;
    bool referenceUseHeader = true;
    inet::Hz referenceBandwidth = inet::Hz(125000);

    mutable long transmissionCount = 0;
    mutable omnetpp::simtime_t accumulatedTimeOnAir;

    omnetpp::simsignal_t transmissionCreatedSignal;
    omnetpp::simsignal_t timeOnAirSignal;

    virtual void initialize(int stage) override;
    virtual void finish() override;

  public:
    LoRaTransmitter() = default;
    virtual std::ostream& printToStream(std::ostream& stream, int level,
            int evFlags = 0) const override;
    virtual const inet::physicallayer::ITransmission *createTransmission(
            const inet::physicallayer::IRadio *radio, const inet::Packet *packet,
            const omnetpp::simtime_t startTime) const override;
};

} // namespace flora

#endif
