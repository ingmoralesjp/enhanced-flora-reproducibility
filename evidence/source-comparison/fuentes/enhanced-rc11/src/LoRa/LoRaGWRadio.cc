#include "LoRaGWRadio.h"

#include <algorithm>
#include <cmath>

#include "LoRaPhy/LoRaMedium.h"
#include "LoRaPhy/LoRaPhyPreamble_m.h"
#include "LoRaTagInfo_m.h"
#include "inet/common/Simsignals.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

namespace flora {

Define_Module(LoRaGWRadio);

bool LoRaGWRadio::afterWarmup() const
{
    return simTime() >= getSimulation()->getWarmupPeriod();
}

void LoRaGWRadio::initialize(int stage)
{
    FlatRadioBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
        gateway = par("iAmGateway").boolValue();
    else if (stage == INITSTAGE_LAST) {
        setRadioMode(RADIO_MODE_TRANSCEIVER);
        receptionStartedSignal = registerSignal("LoRaGWRadioReceptionStarted");
        receptionFinishedCorrectSignal = registerSignal("LoRaGWRadioReceptionFinishedCorrect");
        csiRssiHistogram.setName("Gateway CSI RSSI");
        csiSnrHistogram.setName("Gateway CSI SNR");
    }
}

void LoRaGWRadio::finish()
{
    FlatRadioBase::finish();
    const double der = startedReceptions > 0 ?
            (double)successfulReceptions / startedReceptions : 0;
    recordScalar("DER - Data Extraction Rate", der);
    recordScalar("PktSent", startedReceptions);
    recordScalar("PkTDeliver", successfulReceptions);
    recordScalar("numPKTsSuccesfuls", successfulReceptions);
    recordScalar("numPKTsUnsuccesfuls", unsuccessfulReceptions);
    recordScalar("numPktsBelowSens", belowSensitivity);
    recordScalar("numPktsCollided", collided);
    recordScalar("numPktsLowSnr", lowSnr);
    recordScalar("numPktsLostDuringGWTransmission", ignoredWhileTransmitting);
    recordScalar("numPktsAbortedBeforeEnd", abortedReceptions);
    recordScalar("csiNotifications", csiNotifications);
    csiRssiHistogram.recordAs("gatewayCsiRssi");
    csiSnrHistogram.recordAs("gatewayCsiSnr");
}

void LoRaGWRadio::handleSelfMessage(cMessage *message)
{
    if (message == switchTimer)
        handleSwitchTimer(message);
    else if (isTransmissionTimer(message))
        handleTransmissionTimer(message);
    else if (isReceptionTimer(message))
        handleReceptionTimer(message);
    else
        throw cRuntimeError("Unknown gateway-radio self message '%s'", message->getName());
}

bool LoRaGWRadio::isTransmissionTimer(const cMessage *message) const
{
    return !strcmp(message->getName(), "transmissionTimer");
}

void LoRaGWRadio::handleTransmissionTimer(cMessage *message)
{
    const auto part = (IRadioSignal::SignalPart)message->getKind();
    if (part == IRadioSignal::SIGNAL_PART_WHOLE || part == IRadioSignal::SIGNAL_PART_DATA)
        endTransmission(message);
    else if (part == IRadioSignal::SIGNAL_PART_PREAMBLE || part == IRadioSignal::SIGNAL_PART_HEADER)
        continueTransmission(message);
    else
        throw cRuntimeError("Unknown transmission signal part");
}

void LoRaGWRadio::handleUpperPacket(Packet *packet)
{
    emit(packetReceivedFromUpperSignal, packet);
    const auto& frame = packet->peekAtFront<LoRaMacFrame>();

    auto preamble = makeShared<LoRaPhyPreamble>();
    preamble->setBandwidth(frame->getLoRaBW());
    preamble->setCenterFrequency(frame->getLoRaCF());
    preamble->setCodeRendundance(frame->getLoRaCR());
    preamble->setPower(mW(frame->getLoRaTP()));
    preamble->setSpreadFactor(frame->getLoRaSF());
    preamble->setUseHeader(frame->getLoRaUseHeader());
    preamble->setReceiverAddress(frame->getReceiverAddress());
    preamble->setChunkLength(b(16));
    packet->insertAtFront(preamble);
    packet->addTagIfAbsent<SignalPowerReq>()->setPower(mW(frame->getLoRaTP()));

    startTransmission(packet, separateTransmissionParts ?
            IRadioSignal::SIGNAL_PART_PREAMBLE : IRadioSignal::SIGNAL_PART_WHOLE);
}

void LoRaGWRadio::startTransmission(Packet *packet, IRadioSignal::SignalPart part)
{
    if (transmitting) {
        delete packet;
        return;
    }

    transmitting = true;
    auto signal = createSignal(packet);
    const auto transmission = signal->getTransmission();
    auto timer = new cMessage("transmissionTimer");
    timer->setKind(part);
    timer->setContextPointer(signal);
    scheduleAt(transmission->getEndTime(part), timer);
    emit(transmissionStartedSignal, check_and_cast<const cObject *>(transmission));
    check_and_cast<LoRaMedium *>(medium)->emit(IRadioMedium::signalDepartureStartedSignal,
            check_and_cast<const cObject *>(transmission));
}

void LoRaGWRadio::continueTransmission(cMessage *timer)
{
    const auto nextPart = (IRadioSignal::SignalPart)(timer->getKind() + 1);
    const auto signal = static_cast<IWirelessSignal *>(timer->getContextPointer());
    timer->setKind(nextPart);
    scheduleAt(signal->getTransmission()->getEndTime(nextPart), timer);
}

void LoRaGWRadio::endTransmission(cMessage *timer)
{
    transmitting = false;
    const auto signal = static_cast<WirelessSignal *>(timer->getContextPointer());
    const auto transmission = signal->getTransmission();
    timer->setContextPointer(nullptr);
    emit(transmissionEndedSignal, check_and_cast<const cObject *>(transmission));
    check_and_cast<LoRaMedium *>(medium)->emit(IRadioMedium::signalDepartureEndedSignal,
            check_and_cast<const cObject *>(transmission));
    delete timer;
}

void LoRaGWRadio::handleSignal(WirelessSignal *signal)
{
    auto timer = createReceptionTimer(signal);
    startReception(timer, separateReceptionParts ?
            IRadioSignal::SIGNAL_PART_PREAMBLE : IRadioSignal::SIGNAL_PART_WHOLE);
}

bool LoRaGWRadio::isReceptionTimer(const cMessage *message) const
{
    return !strcmp(message->getName(), "receptionTimer");
}

void LoRaGWRadio::handleReceptionTimer(cMessage *message)
{
    const auto part = (IRadioSignal::SignalPart)message->getKind();
    if (part == IRadioSignal::SIGNAL_PART_WHOLE || part == IRadioSignal::SIGNAL_PART_DATA)
        endReception(message);
    else if (part == IRadioSignal::SIGNAL_PART_PREAMBLE || part == IRadioSignal::SIGNAL_PART_HEADER)
        continueReception(message);
    else
        throw cRuntimeError("Unknown reception signal part");
}

void LoRaGWRadio::startReception(cMessage *timer, IRadioSignal::SignalPart part)
{
    const auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
    const auto arrival = signal->getArrival();
    const auto reception = signal->getReception();
    const auto transmission = signal->getTransmission();

    emit(receptionStartedSignal, true);
    if (afterWarmup())
        startedReceptions++;

    if (isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime() && !transmitting) {
        if (medium->isReceptionAttempted(this, transmission, part)) {
            concurrentReceptions.push_back(timer);
            receptionTimer = timer;
        }
    }
    else if (afterWarmup())
        ignoredWhileTransmitting++;

    timer->setKind(part);
    scheduleAt(arrival->getEndTime(part), timer);
    radioMode = RADIO_MODE_TRANSCEIVER;
    check_and_cast<LoRaMedium *>(medium)->emit(IRadioMedium::signalArrivalStartedSignal,
            check_and_cast<const cObject *>(reception));
}

void LoRaGWRadio::continueReception(cMessage *timer)
{
    const auto previousPart = (IRadioSignal::SignalPart)timer->getKind();
    const auto nextPart = (IRadioSignal::SignalPart)(previousPart + 1);
    const auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
    const auto arrival = signal->getArrival();

    if (std::find(concurrentReceptions.begin(), concurrentReceptions.end(), timer) !=
            concurrentReceptions.end())
        receptionTimer = timer;

    if (timer == receptionTimer && isReceiverMode(radioMode) &&
            arrival->getEndTime(previousPart) == simTime() && !transmitting) {
        const auto transmission = signal->getTransmission();
        if (!medium->isReceptionSuccessful(this, transmission, previousPart) ||
                !medium->isReceptionAttempted(this, transmission, nextPart)) {
            receptionTimer = nullptr;
            concurrentReceptions.remove(timer);
        }
    }

    timer->setKind(nextPart);
    scheduleAt(arrival->getEndTime(nextPart), timer);
    radioMode = RADIO_MODE_TRANSCEIVER;
}

void LoRaGWRadio::endReception(cMessage *timer)
{
    const auto part = (IRadioSignal::SignalPart)timer->getKind();
    const auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
    const auto arrival = signal->getArrival();
    const auto reception = signal->getReception();

    if (std::find(concurrentReceptions.begin(), concurrentReceptions.end(), timer) !=
            concurrentReceptions.end())
        receptionTimer = timer;

    if (timer == receptionTimer && isReceiverMode(radioMode) &&
            arrival->getEndTime() == simTime() && !transmitting) {
        const auto transmission = signal->getTransmission();
        const auto decision = medium->getReceptionDecision(this, signal->getListening(),
                transmission, part);
        const bool possible = decision->isReceptionPossible();
        const bool attempted = decision->isReceptionAttempted();
        const bool successful = decision->isReceptionSuccessful();

        if (successful) {
            auto packet = medium->receivePacket(this, signal);
            take(packet);
            emit(packetSentToUpperSignal, packet);
            emit(receptionFinishedCorrectSignal, true);
            sendUp(packet);
            if (afterWarmup())
                successfulReceptions++;
        }
        else {
            if (afterWarmup()) {
                unsuccessfulReceptions++;
                if (!possible)
                    belowSensitivity++;
                else if (!attempted)
                    collided++;
                else
                    lowSnr++;
            }

            // Only an attempted decode has trustworthy per-frame CSI.
            if (possible && attempted) {
                calculateCsi(reception, signal->getReceiver(), transmission);
                auto notification = createCsiNotification(transmission->getPacket());
                take(notification);
                emit(packetSentToUpperSignal, notification);
                sendUp(notification);
                if (afterWarmup())
                    csiNotifications++;
            }
        }

        receptionTimer = nullptr;
        concurrentReceptions.remove(timer);
    }

    radioMode = RADIO_MODE_TRANSCEIVER;
    check_and_cast<LoRaMedium *>(medium)->emit(IRadioMedium::signalArrivalEndedSignal,
            check_and_cast<const cObject *>(reception));
    delete timer;
}

void LoRaGWRadio::abortReception(cMessage *timer)
{
    const auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
    if (afterWarmup())
        abortedReceptions++;
    concurrentReceptions.remove(timer);
    if (timer == receptionTimer)
        receptionTimer = nullptr;
    if (timer->isScheduled())
        cancelEvent(timer);
    check_and_cast<LoRaMedium *>(medium)->emit(IRadioMedium::signalArrivalEndedSignal,
            check_and_cast<const cObject *>(signal->getReception()));
    delete timer;
    updateTransceiverState();
    updateTransceiverPart();
}

void LoRaGWRadio::calculateCsi(const IReception *reception, const IRadio *receiver,
        const ITransmission *transmission)
{
    const auto loraReception = check_and_cast<const LoRaReception *>(reception);
    csiRssiDbm = math::mW2dBmW(loraReception->getPower().get() * 1000);
    const auto snir = receiver->getMedium()->getSNIR(receiver, transmission);
    csiSnrDb = math::fraction2dB(snir->getMin());
    csiRssiHistogram.collect(csiRssiDbm);
    csiSnrHistogram.collect(csiSnrDb);
}

Packet *LoRaGWRadio::createCsiNotification(const Packet *originalPacket) const
{
    auto notification = originalPacket->dup();
    const auto originalPreamble = notification->removeAtFront<LoRaPhyPreamble>();
    const auto originalFrame = notification->removeAtFront<LoRaMacFrame>();

    auto frame = makeShared<LoRaMacFrame>(*originalFrame);
    frame->setMiRSSI(csiRssiDbm);
    frame->setMiSNIR(csiSnrDb);
    frame->setIsGeneratedByGateway(true);
    notification->insertAtFront(frame);

    auto preamble = makeShared<LoRaPhyPreamble>(*originalPreamble);
    preamble->setPmiRSSI(csiRssiDbm);
    preamble->setPmiSNIR(csiSnrDb);
    notification->insertAtFront(preamble);

    notification->addTagIfAbsent<LoRaTag>()->setIsGeneratedByGateway(true);
    notification->addTagIfAbsent<SignalPowerInd>()->setPower(
            W(pow(10.0, csiRssiDbm / 10.0) * 1e-3));
    notification->addTagIfAbsent<SnirInd>()->setMinimumSnir(
            pow(10.0, csiSnrDb / 10.0));
    return notification;
}

} // namespace flora
