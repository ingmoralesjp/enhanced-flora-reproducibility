#ifndef __FLORA_LORAGWRADIO_H
#define __FLORA_LORAGWRADIO_H

#include <list>

#include "inet/physicallayer/wireless/common/base/packetlevel/FlatRadioBase.h"

#include "LoRaMacFrame_m.h"
#include "LoRaPhy/LoRaReception.h"

namespace flora {

using namespace inet;
using namespace inet::physicallayer;
using namespace omnetpp;

/**
 * Gateway radio with concurrent LoRa receptions and thesis CSI reporting.
 * A CSI notification is emitted only when a reception was possible and
 * attempted but decoding failed; it is never counted as a delivered uplink.
 */
class LoRaGWRadio : public FlatRadioBase
{
  protected:
    bool gateway = false;
    bool transmitting = false;
    std::list<cMessage *> concurrentReceptions;

    long startedReceptions = 0;
    long successfulReceptions = 0;
    long unsuccessfulReceptions = 0;
    long belowSensitivity = 0;
    long collided = 0;
    long lowSnr = 0;
    long ignoredWhileTransmitting = 0;
    long abortedReceptions = 0;
    long csiNotifications = 0;

    double csiRssiDbm = NaN;
    double csiSnrDb = NaN;
    cHistogram csiRssiHistogram;
    cHistogram csiSnrHistogram;

    simsignal_t receptionStartedSignal;
    simsignal_t receptionFinishedCorrectSignal;

  protected:
    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual void handleSelfMessage(cMessage *message) override;
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void handleSignal(WirelessSignal *signal) override;

    virtual bool isTransmissionTimer(const cMessage *message) const;
    virtual void handleTransmissionTimer(cMessage *message) override;
    virtual void startTransmission(Packet *packet, IRadioSignal::SignalPart part) override;
    virtual void continueTransmission(cMessage *timer);
    virtual void endTransmission(cMessage *timer);

    virtual bool isReceptionTimer(const cMessage *message) const override;
    virtual void handleReceptionTimer(cMessage *message) override;
    virtual void startReception(cMessage *timer, IRadioSignal::SignalPart part) override;
    virtual void continueReception(cMessage *timer) override;
    virtual void endReception(cMessage *timer) override;
    virtual void abortReception(cMessage *timer) override;

    void calculateCsi(const IReception *reception, const IRadio *receiver,
            const ITransmission *transmission);
    Packet *createCsiNotification(const Packet *originalPacket) const;
    bool afterWarmup() const;

  public:
    LoRaGWRadio() = default;
};

} // namespace flora

#endif
