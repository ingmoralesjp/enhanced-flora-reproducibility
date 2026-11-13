#include "NetworkServerApp.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <numeric>

#include "inet/common/INETMath.h"
#include "inet/common/ModuleAccess.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3Tools.h"

#include "../LoRaPhy/LoRaAirtimeCalculator.h"

namespace flora {

Define_Module(NetworkServerApp);

namespace {

double clampValue(double value, double minimum, double maximum)
{
    return std::max(minimum, std::min(maximum, value));
}

int clampInt(int value, int minimum, int maximum)
{
    return std::max(minimum, std::min(maximum, value));
}

} // namespace

NetworkServerApp::~NetworkServerApp()
{
    clearPendingUplinks();
    if (getSimulation() != nullptr && getSimulation()->getSystemModule() != nullptr)
        getSimulation()->getSystemModule()->unsubscribe("LoRa_AppPacketSent", this);
}

void NetworkServerApp::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        packetReceivedSignal = registerSignal("LoRa_ServerPacketReceived");
        localPort = par("localPort");
        destPort = par("destPort");
        adrMethod = par("adrMethod").stdstringValue();
        adrDeviceMargin = par("adrDeviceMargin");
        adrStepSize = par("adrStepSize");
        adrWindowSize = par("adrWindowSize");
        proposalMaxDistance = par("proposalMaxDistance");
        enableM1 = par("enableM1");
        enableM2 = par("enableM2");
        forcePeriodicAdrResponse = par("forcePeriodicAdrResponse");

        adaptiveMobilityAdr = par("adaptiveMobilityAdr");
        useCsiNotificationsForAdr = par("useCsiNotificationsForAdr");
        adrAlgorithmName = par("adrAlgorithm").stdstringValue();
        if (adrAlgorithmName == "compatibility") {
            adrAlgorithm = adaptiveMobilityAdr ? AdrAlgorithm::MOBILITY_ENERGY :
                    AdrAlgorithm::THESIS_LEGACY;
            adrAlgorithmName = adaptiveMobilityAdr ? "mobilityEnergy" : "thesisLegacy";
        }
        else if (adrAlgorithmName == "lorawanReference")
            adrAlgorithm = AdrAlgorithm::LORAWAN_REFERENCE;
        else if (adrAlgorithmName == "thesisLegacy")
            adrAlgorithm = AdrAlgorithm::THESIS_LEGACY;
        else if (adrAlgorithmName == "mobilityEnergy")
            adrAlgorithm = AdrAlgorithm::MOBILITY_ENERGY;
        else if (adrAlgorithmName == "ndAdr")
            adrAlgorithm = AdrAlgorithm::ND_ADR;
        else if (adrAlgorithmName == "madere")
            adrAlgorithm = AdrAlgorithm::MADERE;
        else
            throw cRuntimeError("adrAlgorithm must be lorawanReference, thesisLegacy, mobilityEnergy, ndAdr, madere or compatibility");
        staticAdrWindowSize = par("staticAdrWindowSize");
        mobileAdrWindowSize = par("mobileAdrWindowSize");
        ewmaAlpha = par("ewmaAlpha");
        mobilitySpeedThreshold = par("mobilitySpeedThreshold");
        mobileConfirmationSamples = par("mobileConfirmationSamples");
        staticConfirmationSamples = par("staticConfirmationSamples");
        staticAdrMargin = par("staticAdrMargin");
        mobileAdrMargin = par("mobileAdrMargin");
        sfBoundaryHysteresis = par("sfBoundaryHysteresis");
        sfConfirmationSamples = par("sfConfirmationSamples");
        targetPdr = par("targetPdr");
        reliabilitySlope = par("reliabilitySlope");
        channelLoadPenalty = par("channelLoadPenalty");
        collisionModelScale = par("collisionModelScale");
        channelSwitchLoadHysteresis = par("channelSwitchLoadHysteresis");
        constrainMobileSfByDistance = par("constrainMobileSfByDistance");
        energyEvaluationPeriod = par("energyEvaluationPeriod");
        rxWindowDuration = par("rxWindowDuration");
        rxWindowCount = par("rxWindowCount");
        stableHeartbeatInterval = par("stableHeartbeatInterval").doubleValue();
        activeHeartbeatInterval = par("activeHeartbeatInterval").doubleValue();
        initialHeartbeatInterval = par("initialHeartbeatInterval").doubleValue();
        deteriorationSnrDrop = par("deteriorationSnrDrop");
        deteriorationRssiDrop = par("deteriorationRssiDrop");
        deteriorationConfirmationSamples = par("deteriorationConfirmationSamples");
        deteriorationHoldSamples = par("deteriorationHoldSamples");
        maxTpDecreasePerCommand = par("maxTpDecreasePerCommand");
        maxTpIncreasePerCommand = par("maxTpIncreasePerCommand");

        if (adrMethod != "max" && adrMethod != "avg")
            throw cRuntimeError("adrMethod must be 'max' or 'avg'");
        if (adrStepSize <= 0)
            throw cRuntimeError("adrStepSize must be positive");
        if (adrWindowSize <= 0 || staticAdrWindowSize <= 0 || mobileAdrWindowSize <= 0)
            throw cRuntimeError("All ADR window sizes must be positive");
        if (proposalMaxDistance <= 0 || reliabilitySlope <= 0 || energyEvaluationPeriod <= 0 ||
                collisionModelScale <= 0)
            throw cRuntimeError("ADR distances, reliability slope and energy period must be positive");
        if (ewmaAlpha <= 0 || ewmaAlpha > 1 || targetPdr <= 0 || targetPdr > 1)
            throw cRuntimeError("ewmaAlpha and targetPdr must be in (0,1]");
        if (mobileConfirmationSamples <= 0 || staticConfirmationSamples <= 0 ||
                sfConfirmationSamples <= 0 || deteriorationConfirmationSamples <= 0 ||
                deteriorationHoldSamples < 0 || rxWindowCount < 0)
            throw cRuntimeError("ADR confirmation counts must be positive and RX count non-negative");
        if (stableHeartbeatInterval < 0 || activeHeartbeatInterval < 0 || initialHeartbeatInterval < 0)
            throw cRuntimeError("Heartbeat intervals cannot be negative");

        if (adrAlgorithm == AdrAlgorithm::MOBILITY_ENERGY)
            loadEnergyConfiguration();
        receivedRssi.setName("Received RSSI");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        startUdp();
        evaluateAdr = par("evaluateADRinServer");
        getSimulation()->getSystemModule()->subscribe("LoRa_AppPacketSent", this);
    }
}

void NetworkServerApp::loadEnergyConfiguration()
{
    cXMLElement *root = par("energyConfigFile").xmlValue();
    if (root == nullptr)
        throw cRuntimeError("adaptiveMobilityAdr requires energyConfigFile");

    auto readRequired = [root](const char *tagName) {
        const auto elements = root->getElementsByTagName(tagName);
        if (elements.empty() || elements.front()->getAttribute("value") == nullptr)
            throw cRuntimeError("%s not defined in ADR energy configuration", tagName);
        return std::strtod(elements.front()->getAttribute("value"), nullptr);
    };
    supplyVoltage = readRequired("supplyVoltage");
    receiverCurrentMa = readRequired("receiverReceivingSupplyCurrent");
    sleepCurrentMa = readRequired("sleepSupplyCurrent");

    const auto containers = root->getElementsByTagName("txSupplyCurrents");
    if (containers.empty())
        throw cRuntimeError("txSupplyCurrents not defined in ADR energy configuration");
    for (auto element : containers.front()->getElementsByTagName("txSupplyCurrent")) {
        const char *power = element->getAttribute("txPower");
        const char *current = element->getAttribute("supplyCurrent");
        if (power == nullptr || current == nullptr)
            throw cRuntimeError("Each txSupplyCurrent needs txPower and supplyCurrent");
        txCurrentMaByDbm[std::lround(std::strtod(power, nullptr))] =
                std::strtod(current, nullptr);
    }
    if (txCurrentMaByDbm.empty())
        throw cRuntimeError("No TX supply currents defined for adaptive ADR");
}

double NetworkServerApp::getTxCurrentMa(int txPowerDbm) const
{
    auto upper = txCurrentMaByDbm.lower_bound(txPowerDbm);
    if (upper == txCurrentMaByDbm.begin())
        return upper->second;
    if (upper == txCurrentMaByDbm.end())
        return std::prev(upper)->second;
    auto lower = std::prev(upper);
    return txPowerDbm - lower->first <= upper->first - txPowerDbm ? lower->second : upper->second;
}

void NetworkServerApp::startUdp()
{
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
}

void NetworkServerApp::handleMessage(cMessage *message)
{
    if (message->arrivedOn("socketIn"))
        handleUplink(check_and_cast<Packet *>(message));
    else if (message->isSelfMessage())
        processPendingUplink(message);
    else
        throw cRuntimeError("Unexpected message '%s'", message->getName());
}

void NetworkServerApp::handleUplink(Packet *packet)
{
    const auto& frame = packet->peekAtFront<LoRaMacFrame>();
    std::unique_ptr<Packet> payloadView(packet->dup());
    payloadView->trimFront();
    payloadView->removeAtFront<LoRaMacFrame>();
    const auto& appPacket = payloadView->peekAtFront<LoRaAppPacket>();

    const bool heartbeat = appPacket->getOptions().isHeartbeat();
    const bool csi = frame->isGeneratedByGateway();
    if (simTime() >= getSimulation()->getWarmupPeriod()) {
        totalReceivedCopies++;
        heartbeatCopies += heartbeat;
        csiNotifications += csi;
    }

    // CSI is an enhanced-simulator observation, not an ordinary LoRaWAN
    // uplink. Keeping this gate treatment-specific prevents the reference and
    // legacy algorithms from receiving information unavailable to them.
    if (csi && !useCsiNotificationsForAdr) {
        delete packet;
        return;
    }

    updateKnownNode(frame);
    if (isOldPacket(frame)) {
        delete packet;
        return;
    }
    addToProcessingTable(packet);
}

KnownNode& NetworkServerApp::updateKnownNode(const Ptr<const LoRaMacFrame>& frame)
{
    const int index = findNode(frame->getTransmitterAddress());
    if (index >= 0) {
        auto& node = knownNodes[index];
        node.lastSequenceNumber = std::max(node.lastSequenceNumber, frame->getSequenceNumber());
        node.currentSf = frame->getLoRaSF();
        node.currentPowerDbm = math::mW2dBmW(frame->getLoRaTP()) + 30;
        node.currentChannelMhz = frame->getLoRaCF().get() / 1e6;
        return node;
    }

    KnownNode node;
    node.address = frame->getTransmitterAddress();
    node.lastSequenceNumber = frame->getSequenceNumber();
    node.currentSf = frame->getLoRaSF();
    node.currentPowerDbm = math::mW2dBmW(frame->getLoRaTP()) + 30;
    node.currentChannelMhz = frame->getLoRaCF().get() / 1e6;
    node.commandedHeartbeatInterval = initialHeartbeatInterval;
    knownNodes.push_back(node);
    return knownNodes.back();
}

bool NetworkServerApp::isOldPacket(const Ptr<const LoRaMacFrame>& frame) const
{
    const int index = findNode(frame->getTransmitterAddress());
    // Equal-sequence copies may join an active pending entry. After that entry
    // is processed, a late copy must not create a second delivery/PDR sample.
    return index >= 0 &&
            frame->getSequenceNumber() <= knownNodes[index].lastProcessedSequenceNumber;
}

int NetworkServerApp::findNode(const MacAddress& address) const
{
    for (size_t i = 0; i < knownNodes.size(); ++i)
        if (knownNodes[i].address == address)
            return i;
    return -1;
}

int NetworkServerApp::findPendingUplink(const Ptr<const LoRaMacFrame>& frame) const
{
    for (size_t i = 0; i < pendingUplinks.size(); ++i) {
        const auto& pendingFrame = pendingUplinks[i].packet->peekAtFront<LoRaMacFrame>();
        if (pendingFrame->getTransmitterAddress() == frame->getTransmitterAddress() &&
                pendingFrame->getSequenceNumber() == frame->getSequenceNumber())
            return i;
    }
    return -1;
}

void NetworkServerApp::addToProcessingTable(Packet *packet)
{
    const auto& frame = packet->peekAtFront<LoRaMacFrame>();
    const auto& networkHeader = getNetworkProtocolHeader(packet);
    const L3Address gateway = networkHeader->getSourceAddress();
    const double snrDb = math::fraction2dB(frame->getSNIR());

    const int pendingIndex = findPendingUplink(frame);
    if (pendingIndex >= 0) {
        pendingUplinks[pendingIndex].gateways.emplace_back(gateway, snrDb, frame->getRSSI());
        delete packet;
        return;
    }

    PendingUplink pending;
    pending.packet = packet;
    pending.waitingTimer = new cMessage("endOfWaitingWindow");
    pending.waitingTimer->setContextPointer(packet);
    pending.gateways.emplace_back(gateway, snrDb, frame->getRSSI());
    scheduleAt(simTime() + 1.2, pending.waitingTimer);
    pendingUplinks.push_back(pending);
}

void NetworkServerApp::processPendingUplink(cMessage *timer)
{
    int pendingIndex = -1;
    for (size_t i = 0; i < pendingUplinks.size(); ++i)
        if (pendingUplinks[i].waitingTimer == timer) {
            pendingIndex = i;
            break;
        }
    if (pendingIndex < 0)
        throw cRuntimeError("ADR waiting timer is not associated with an uplink");

    auto& pending = pendingUplinks[pendingIndex];
    Packet *packet = pending.packet;
    const auto& frame = packet->peekAtFront<LoRaMacFrame>();
    std::unique_ptr<Packet> payloadView(packet->dup());
    payloadView->trimFront();
    payloadView->removeAtFront<LoRaMacFrame>();
    const auto& appPacket = payloadView->peekAtFront<LoRaAppPacket>();

    auto bestGateway = std::max_element(pending.gateways.begin(), pending.gateways.end(),
            [](const auto& a, const auto& b) { return std::get<1>(a) < std::get<1>(b); });
    const L3Address gateway = std::get<0>(*bestGateway);
    const double bestSnr = std::get<1>(*bestGateway);
    const double bestRssi = std::get<2>(*bestGateway);
    const bool accountingPacket = isAccountingPacket(frame, appPacket);

    // Use the uplink timestamp on both sides of the warm-up boundary; this is
    // the correction that prevents an artificial DER/PDR greater than one.
    if (appPacket->getOptions().getSentAt() >= getSimulation()->getWarmupPeriod() && accountingPacket) {
        uniqueDataPackets++;
        if (frame->getLoRaSF() >= 7 && frame->getLoRaSF() <= 12)
            uniqueDataPacketsPerSf[frame->getLoRaSF() - 7]++;
        int receivedChannel = 0;
        const double receivedMhz = frame->getLoRaCF().get() / 1e6;
        for (size_t channel = 1; channel < eu868ChannelsMhz.size(); ++channel)
            if (std::abs(receivedMhz - eu868ChannelsMhz[channel]) <
                    std::abs(receivedMhz - eu868ChannelsMhz[receivedChannel]))
                receivedChannel = channel;
        uniqueDataPacketsPerChannel[receivedChannel]++;
        const int nodeId = frame->getTransmitterAddress().getInt() - 1;
        receivedPerNode[nodeId]++;
        sentPerNode[nodeId] = std::max(1, appPacket->getOptions().getPacketCounter() + 1);
        lastSnrPerNode[nodeId] = bestSnr;
        lastRssiPerNode[nodeId] = bestRssi;
        receivedRssi.collect(bestRssi);
        emit(packetReceivedSignal, true);
    }

    auto& node = knownNodes[findNode(frame->getTransmitterAddress())];
    if (evaluateAdr)
        evaluateAdrCommand(packet, node, gateway, bestSnr, bestRssi);
    if (appPacket->getOptions().getIsconfirmedMSG())
        sendConfirmedAck(frame, appPacket, gateway);
    node.lastProcessedSequenceNumber = std::max(node.lastProcessedSequenceNumber,
            frame->getSequenceNumber());

    delete packet;
    delete timer;
    pendingUplinks.erase(pendingUplinks.begin() + pendingIndex);
}

void NetworkServerApp::evaluateAdrCommand(Packet *packet, KnownNode& node,
        const L3Address& gateway, double bestSnr, double bestRssi)
{
    switch (adrAlgorithm) {
        case AdrAlgorithm::LORAWAN_REFERENCE:
            evaluateLoRaWanReferenceAdrCommand(packet, node, gateway, bestSnr);
            break;
        case AdrAlgorithm::THESIS_LEGACY:
            evaluateThesisLegacyAdrCommand(packet, node, gateway, bestSnr);
            break;
        case AdrAlgorithm::MOBILITY_ENERGY:
            evaluateAdaptiveAdrCommand(packet, node, gateway, bestSnr, bestRssi);
            break;
        case AdrAlgorithm::ND_ADR:
            evaluateNdAdrCommand(packet, node, gateway, bestSnr, bestRssi);
            break;
        case AdrAlgorithm::MADERE:
            evaluateMadereCommand(packet, node, gateway, bestSnr);
            break;
    }
}

void NetworkServerApp::evaluateLoRaWanReferenceAdrCommand(Packet *packet, KnownNode& node,
        const L3Address& gateway, double bestSnr)
{
    const auto& frame = packet->peekAtFront<LoRaMacFrame>();
    std::unique_ptr<Packet> payloadView(packet->dup());
    payloadView->trimFront();
    payloadView->removeAtFront<LoRaMacFrame>();
    const auto& appPacket = payloadView->peekAtFront<LoRaAppPacket>();

    node.snrHistory.push_back(bestSnr);
    while ((int)node.snrHistory.size() > adrWindowSize)
        node.snrHistory.pop_front();
    node.framesSinceAdr++;

    const bool adrAckRequested = appPacket->getOptions().getADRACKReq();
    if (node.framesSinceAdr < adrWindowSize && !adrAckRequested)
        return;
    node.framesSinceAdr = 0;

    // The reference NS-ADR uses a configurable margin divisor. The fidelity
    // ablation campaign fixes it at 2.5 dB to reproduce The Things Stack;
    // older campaign INI files retain the historical 3-dB default.
    const double representativeSnr = adrMethod == "max" ?
            *std::max_element(node.snrHistory.begin(), node.snrHistory.end()) :
            std::accumulate(node.snrHistory.begin(), node.snrHistory.end(), 0.0) /
                    node.snrHistory.size();
    const double marginDb = representativeSnr - requiredSnr(node.currentSf) - adrDeviceMargin;
    int remainingSteps = static_cast<int>(marginDb / adrStepSize);
    int proposedSf = node.currentSf;
    int proposedPowerDbm = clampInt((int)std::round(node.currentPowerDbm), 2, 14);

    while (remainingSteps > 0) {
        if (proposedSf > 7)
            proposedSf--; // Higher data rate is preferred before reducing TP.
        else if (proposedPowerDbm > 2)
            proposedPowerDbm = std::max(2, proposedPowerDbm - 3);
        else
            break;
        remainingSteps--;
    }
    while (remainingSteps < 0) {
        if (proposedPowerDbm < 14)
            proposedPowerDbm = std::min(14, proposedPowerDbm + 3);
        else
            break; // Further robustness is handled by ED ADR recovery.
        remainingSteps++; // Consume a negative step toward zero.
    }

    int currentChannelIndex = 0;
    for (size_t i = 1; i < eu868ChannelsMhz.size(); ++i)
        if (std::abs(eu868ChannelsMhz[i] - node.currentChannelMhz) <
                std::abs(eu868ChannelsMhz[currentChannelIndex] - node.currentChannelMhz))
            currentChannelIndex = i;

    // NStep=0 produces no command, except that ADRACKReq must receive a reply
    // so the end device can reset ADRACKCnt as specified by LoRaWAN Class A.
    sendAdrCommand(frame, node, gateway, proposedSf, proposedPowerDbm,
            currentChannelIndex, -1, -1, adrAckRequested);
}

void NetworkServerApp::evaluateThesisLegacyAdrCommand(Packet *packet, KnownNode& node,
        const L3Address& gateway, double bestSnr)
{
    const auto& frame = packet->peekAtFront<LoRaMacFrame>();
    std::unique_ptr<Packet> payloadView(packet->dup());
    payloadView->trimFront();
    payloadView->removeAtFront<LoRaMacFrame>();
    const auto& appPacket = payloadView->peekAtFront<LoRaAppPacket>();

    node.snrHistory.push_back(bestSnr);
    while ((int)node.snrHistory.size() > adrWindowSize)
        node.snrHistory.pop_front();
    node.framesSinceAdr++;

    const bool adrAckRequested = appPacket->getOptions().getADRACKReq();
    if (node.framesSinceAdr < adrWindowSize && !adrAckRequested)
        return;
    node.framesSinceAdr = 0;

    double representativeSnr = adrMethod == "max" ?
            *std::max_element(node.snrHistory.begin(), node.snrHistory.end()) :
            std::accumulate(node.snrHistory.begin(), node.snrHistory.end(), 0.0) / node.snrHistory.size();
    int proposedSf = node.currentSf;
    int proposedPowerDbm = clampInt((int)std::round(node.currentPowerDbm), 2, 14);
    int marginSteps = static_cast<int>((representativeSnr - requiredSnr(node.currentSf) -
            adrDeviceMargin) / adrStepSize);
    while (marginSteps > 0) {
        if (proposedSf > 7)
            proposedSf--;
        else if (proposedPowerDbm > 2)
            proposedPowerDbm = std::max(2, proposedPowerDbm - 3);
        else
            break;
        marginSteps--;
    }
    while (marginSteps < 0 && proposedPowerDbm < 14) {
        proposedPowerDbm = std::min(14, proposedPowerDbm + 3);
        marginSteps++;
    }

    if (enableM2) {
        if (!appPacket->getOptions().getHasPosition())
            throw cRuntimeError("M2 requires includePosition=true at every end device");
        const double distance = std::max(0.0, appPacket->getOptions().getDistanceED());
        const double bandWidth = proposalMaxDistance / 6.0;
        proposedSf = 6 + clampInt((int)std::ceil(distance / bandWidth), 1, 6);
    }

    int currentChannelIndex = 0;
    for (size_t i = 1; i < eu868ChannelsMhz.size(); ++i)
        if (std::abs(eu868ChannelsMhz[i] - node.currentChannelMhz) <
                std::abs(eu868ChannelsMhz[currentChannelIndex] - node.currentChannelMhz))
            currentChannelIndex = i;
    const int proposedChannel = enableM1 ? findLeastLoadedChannel() : currentChannelIndex;

    // forcePeriodicAdrResponse=true reproduces the historical implementation;
    // corrected experiments suppress no-op downlinks except for ADRACKReq.
    sendAdrCommand(frame, node, gateway, proposedSf, proposedPowerDbm,
            proposedChannel, -1, -1, forcePeriodicAdrResponse || adrAckRequested);
}

void NetworkServerApp::evaluateNdAdrCommand(Packet *packet, KnownNode& node,
        const L3Address& gateway, double bestSnr, double bestRssi)
{
    const auto& frame = packet->peekAtFront<LoRaMacFrame>();
    std::unique_ptr<Packet> payloadView(packet->dup());
    payloadView->trimFront();
    payloadView->removeAtFront<LoRaMacFrame>();
    const auto& appPacket = payloadView->peekAtFront<LoRaAppPacket>();
    const bool adrAckRequested = appPacket->getOptions().getADRACKReq();
    if (!adrAckRequested)
        return;

    node.ndSnrHistory.push_back(bestSnr);
    while (node.ndSnrHistory.size() > 15)
        node.ndSnrHistory.pop_front();

    int currentChannelIndex = 0;
    for (size_t i = 1; i < eu868ChannelsMhz.size(); ++i)
        if (std::abs(eu868ChannelsMhz[i] - node.currentChannelMhz) <
                std::abs(eu868ChannelsMhz[currentChannelIndex] - node.currentChannelMhz))
            currentChannelIndex = i;

    if ((int)node.ndSnrHistory.size() < node.ndWindow) {
        sendAdrCommand(frame, node, gateway, node.currentSf,
                clampInt((int)std::round(node.currentPowerDbm), 2, 14),
                currentChannelIndex, -1, -1, true);
        return;
    }

    const double averageSnr = std::accumulate(
            node.ndSnrHistory.end() - node.ndWindow, node.ndSnrHistory.end(), 0.0) /
            node.ndWindow;
    static constexpr double snrThresholds[6] = {-7.5, -10, -12.5, -15, -17.5, -20};
    static constexpr double rssiThresholds[6] = {-123, -126, -129, -132, -134.5, -137};
    auto thresholdSf = [](double value, const double *thresholds) {
        for (int sf = 7; sf <= 12; ++sf)
            if (value >= thresholds[sf - 7])
                return sf;
        return 12;
    };
    const int snrSf = thresholdSf(averageSnr, snrThresholds);
    const int rssiSf = thresholdSf(bestRssi, rssiThresholds);
    if (snrSf != rssiSf) {
        node.ndWindow = std::min(node.ndWindow + 1, 15);
        sendAdrCommand(frame, node, gateway, node.currentSf,
                clampInt((int)std::round(node.currentPowerDbm), 2, 14),
                currentChannelIndex, -1, -1, true);
        return;
    }

    node.ndWindow = 3;
    node.ndDecisions++;
    int proposedSf = node.currentSf;
    int proposedPowerDbm = clampInt((int)std::round(node.currentPowerDbm), 2, 14);
    int steps = static_cast<int>(std::round(
            (averageSnr - requiredSnr(node.currentSf) - adrDeviceMargin) / adrStepSize));
    while (steps > 0) {
        if (proposedSf > 7)
            proposedSf--;
        else if (proposedPowerDbm > 2)
            proposedPowerDbm = std::max(2, proposedPowerDbm - 3);
        else
            break;
        steps--;
    }
    while (steps < 0 && proposedPowerDbm < 14) {
        proposedPowerDbm = std::min(14, proposedPowerDbm + 3);
        steps++;
    }
    sendAdrCommand(frame, node, gateway, proposedSf, proposedPowerDbm,
            currentChannelIndex, -1, -1, true);
}

void NetworkServerApp::evaluateMadereCommand(Packet *packet, KnownNode& node,
        const L3Address& gateway, double bestSnr)
{
    const auto& frame = packet->peekAtFront<LoRaMacFrame>();
    int proposedSf = node.currentSf;
    int proposedPowerDbm = clampInt((int)std::round(node.currentPowerDbm), 2, 14);
    int steps = static_cast<int>(std::round(
            (bestSnr - requiredSnr(node.currentSf) - adrDeviceMargin) / adrStepSize));
    while (steps > 0) {
        if (proposedSf > 7)
            proposedSf--;
        else if (proposedPowerDbm > 2)
            proposedPowerDbm = std::max(2, proposedPowerDbm - 3);
        else
            break;
        steps--;
    }
    while (steps < 0 && proposedPowerDbm < 14) {
        proposedPowerDbm = std::min(14, proposedPowerDbm + 3);
        steps++;
    }
    int currentChannelIndex = 0;
    for (size_t i = 1; i < eu868ChannelsMhz.size(); ++i)
        if (std::abs(eu868ChannelsMhz[i] - node.currentChannelMhz) <
                std::abs(eu868ChannelsMhz[currentChannelIndex] - node.currentChannelMhz))
            currentChannelIndex = i;
    // The ED applies its position-uncertainty SF after receiving the SNR. TP
    // remains the conventional ADR result, matching the recovered baseline.
    sendAdrCommand(frame, node, gateway, proposedSf, proposedPowerDbm,
            currentChannelIndex, -1, -1, true, bestSnr);
}

void NetworkServerApp::updateAdaptiveObservation(KnownNode& node,
        const Ptr<const LoRaMacFrame>& frame, const Ptr<const LoRaAppPacket>& appPacket,
        double bestSnr, double bestRssi)
{
    const auto& options = appPacket->getOptions();
    const bool failedReception = frame->isGeneratedByGateway();
    const int sequence = frame->getSequenceNumber();
    double deliverySample = failedReception ? 0.0 : 1.0;
    if (node.lastAdaptiveSequenceNumber >= 0 && sequence > node.lastAdaptiveSequenceNumber)
        deliverySample = failedReception ? 0.0 : 1.0 / (sequence - node.lastAdaptiveSequenceNumber);
    node.lastAdaptiveSequenceNumber = std::max(node.lastAdaptiveSequenceNumber, sequence);

    bool deteriorationSample = failedReception;
    if (node.linkEwmaInitialized) {
        deteriorationSample = deteriorationSample || bestSnr < node.ewmaSnr - deteriorationSnrDrop ||
                bestRssi < node.ewmaRssi - deteriorationRssiDrop;
        node.ewmaSnr = ewmaAlpha * bestSnr + (1 - ewmaAlpha) * node.ewmaSnr;
        node.ewmaRssi = ewmaAlpha * bestRssi + (1 - ewmaAlpha) * node.ewmaRssi;
        node.ewmaDelivery = ewmaAlpha * deliverySample + (1 - ewmaAlpha) * node.ewmaDelivery;
    }
    else {
        node.ewmaSnr = bestSnr;
        node.ewmaRssi = bestRssi;
        node.ewmaDelivery = deliverySample;
        node.linkEwmaInitialized = true;
    }
    // Log-normal shadowing can produce an isolated SNR/RSSI drop even for a
    // perfectly static node. Require consecutive evidence before switching to
    // the short heartbeat; otherwise fading creates a costly short/long
    // feedback loop. Once confirmed, keep the alert for a bounded clean-sample
    // horizon so a real deterioration is not forgotten immediately.
    if (deteriorationSample) {
        node.deteriorationEvidence++;
        if (node.deteriorationEvidence >= deteriorationConfirmationSamples) {
            node.deteriorationSamplesRemaining = deteriorationHoldSamples;
            node.deteriorationEvidence = 0;
            node.deteriorationEvents++;
        }
    }
    else {
        node.deteriorationEvidence = 0;
        if (node.deteriorationSamplesRemaining > 0)
            node.deteriorationSamplesRemaining--;
    }

    node.snrHistory.push_back(bestSnr);
    while ((int)node.snrHistory.size() > staticAdrWindowSize)
        node.snrHistory.pop_front();

    // Motion is inferred only from coordinate displacement over elapsed time.
    // RSSI/SNR deterioration changes heartbeat policy but not mobility class.
    if (options.getHasPosition()) {
        if (node.hasPosition && simTime() > node.lastObservationTime) {
            const double dx = options.getPositionX() - node.lastPositionX;
            const double dy = options.getPositionY() - node.lastPositionY;
            const double speed = std::sqrt(dx * dx + dy * dy) /
                    (simTime() - node.lastObservationTime).dbl();
            node.ewmaSpeed = ewmaAlpha * speed + (1 - ewmaAlpha) * node.ewmaSpeed;
            if (node.ewmaSpeed >= mobilitySpeedThreshold) {
                node.movingEvidence++;
                node.stationaryEvidence = 0;
            }
            else {
                node.stationaryEvidence++;
                node.movingEvidence = 0;
            }

            const int previousClass = node.mobilityClass;
            if (node.movingEvidence >= mobileConfirmationSamples)
                node.mobilityClass = ADR_MOBILITY_MOBILE;
            else if (node.stationaryEvidence >= staticConfirmationSamples)
                node.mobilityClass = ADR_MOBILITY_STATIC;
            if (previousClass != node.mobilityClass)
                node.mobilityTransitions++;
        }
        node.hasPosition = true;
        node.lastPositionX = options.getPositionX();
        node.lastPositionY = options.getPositionY();
        node.lastObservationTime = simTime();
    }
}

double NetworkServerApp::predictSuccess(const KnownNode& node,
        const Ptr<const LoRaMacFrame>& frame, int payloadBytes, int sf, int tpDbm,
        int channelIndex, const std::vector<int>& loads) const
{
    // TP changes shift received SNR by the same dB amount in the link-budget
    // approximation. The logistic curve maps margin to link probability.
    const double candidateSnr = node.ewmaSnr + tpDbm - node.currentPowerDbm;
    const double margin = candidateSnr - requiredSnr(sf);
    const double linkProbability = 1.0 / (1.0 + std::exp(-margin / reliabilitySlope));

    const double currentMargin = node.ewmaSnr - requiredSnr(node.currentSf);
    const double currentLinkProbability = 1.0 / (1.0 + std::exp(-currentMargin / reliabilitySlope));
    int currentChannel = 0;
    for (size_t i = 1; i < eu868ChannelsMhz.size(); ++i)
        if (std::abs(eu868ChannelsMhz[i] - node.currentChannelMhz) <
                std::abs(eu868ChannelsMhz[currentChannel] - node.currentChannelMhz))
            currentChannel = i;
    // Only competing nodes contribute collision pressure. Counting the node
    // itself made an otherwise empty channel fail the target-PDR constraint.
    const int candidateContenders = std::max(0, loads[channelIndex] -
            (channelIndex == currentChannel ? 1 : 0));
    const int currentContenders = std::max(0, loads[currentChannel] - 1);
    const LoRaAirtime candidateAirtime = calculateLoRaAirtime(sf,
            frame->getLoRaBW().get(), payloadBytes, frame->getLoRaCR(), true);
    const LoRaAirtime currentAirtime = calculateLoRaAirtime(node.currentSf,
            frame->getLoRaBW().get(), payloadBytes, frame->getLoRaCR(), true);

    // Unslotted ALOHA: P(no overlap) ~= exp(-2G). Unlike the former normalized
    // node-count penalty, this term distinguishes the collision exposure of
    // SF7 from the much longer ToA at SF12.
    const double candidateCollision = std::exp(-collisionModelScale * candidateContenders *
            candidateAirtime.total().dbl() / energyEvaluationPeriod);
    const double currentCollision = std::exp(-collisionModelScale * currentContenders *
            currentAirtime.total().dbl() / energyEvaluationPeriod);
    const double unexplainedLoss = clampValue(node.ewmaDelivery /
            std::max(0.05, currentLinkProbability * currentCollision), 0.1, 1.0);
    return clampValue(linkProbability * candidateCollision * unexplainedLoss, 0.001, 1.0);
}

double NetworkServerApp::estimateEnergyCost(const Ptr<const LoRaMacFrame>& frame,
        int payloadBytes, int sf, int tpDbm, double successProbability) const
{
    const LoRaAirtime airtime = calculateLoRaAirtime(sf, frame->getLoRaBW().get(),
            payloadBytes, frame->getLoRaCR(), true);

    // Voltage[V] * current[mA] * time[s] / 1000 gives joules.
    const double txEnergy = supplyVoltage * getTxCurrentMa(tpDbm) * airtime.total().dbl() / 1000.0;
    const double rxEnergy = supplyVoltage * receiverCurrentMa *
            rxWindowDuration * rxWindowCount / 1000.0;
    const double sleepSeconds = std::max(0.0, energyEvaluationPeriod -
            airtime.total().dbl() - rxWindowDuration * rxWindowCount);
    const double sleepEnergy = supplyVoltage * sleepCurrentMa * sleepSeconds / 1000.0;
    return (txEnergy + rxEnergy + sleepEnergy) / std::max(0.001, successProbability);
}

std::vector<int> NetworkServerApp::channelLoads() const
{
    std::vector<int> loads(eu868ChannelsMhz.size(), 0);
    for (const auto& knownNode : knownNodes) {
        auto channel = std::min_element(eu868ChannelsMhz.begin(), eu868ChannelsMhz.end(),
                [&knownNode](double a, double b) {
                    return std::abs(a - knownNode.currentChannelMhz) <
                            std::abs(b - knownNode.currentChannelMhz);
                });
        loads[std::distance(eu868ChannelsMhz.begin(), channel)]++;
    }
    return loads;
}

AdrCandidate NetworkServerApp::selectEnergyCandidate(const KnownNode& node,
        const Ptr<const LoRaMacFrame>& frame, int payloadBytes,
        int fixedSpreadingFactor, double safetyMargin) const
{
    const auto loads = channelLoads();
    AdrCandidate best;
    best.energyCostJ = std::numeric_limits<double>::infinity();
    AdrCandidate fallback = best;
    double highestSuccess = -1;
    int currentChannel = 0;
    for (size_t i = 1; i < eu868ChannelsMhz.size(); ++i)
        if (std::abs(eu868ChannelsMhz[i] - node.currentChannelMhz) <
                std::abs(eu868ChannelsMhz[currentChannel] - node.currentChannelMhz))
            currentChannel = i;

    const int firstSf = fixedSpreadingFactor < 0 ? 7 : fixedSpreadingFactor;
    const int lastSf = fixedSpreadingFactor < 0 ? 12 : fixedSpreadingFactor;
    for (int sf = firstSf; sf <= lastSf; ++sf) {
        for (int tp = 2; tp <= 14; ++tp) {
            for (size_t channel = 0; channel < eu868ChannelsMhz.size(); ++channel) {
                // Do not move channels for a one-node/noise-level difference;
                // a real load improvement must exceed the configured hysteresis.
                if ((int)channel != currentChannel &&
                        loads[currentChannel] - loads[channel] <= channelSwitchLoadHysteresis)
                    continue;

                const double predictedSuccess = predictSuccess(node, frame, payloadBytes,
                        sf, tp, channel, loads);
                const double margin = node.ewmaSnr + tp - node.currentPowerDbm - requiredSnr(sf);
                const double cost = estimateEnergyCost(frame, payloadBytes, sf, tp, predictedSuccess);
                AdrCandidate candidate{sf, tp, (int)channel, predictedSuccess, margin, cost,
                        predictedSuccess >= targetPdr && margin >= safetyMargin};
                if (candidate.feasible && candidate.energyCostJ < best.energyCostJ)
                    best = candidate;
                if (predictedSuccess > highestSuccess ||
                        (predictedSuccess == highestSuccess && cost < fallback.energyCostJ)) {
                    fallback = candidate;
                    highestSuccess = predictedSuccess;
                }
            }
        }
    }
    return std::isfinite(best.energyCostJ) ? best : fallback;
}

int NetworkServerApp::confirmedMobileSpreadingFactor(KnownNode& node, double distance)
{
    const double width = proposalMaxDistance / 6.0;
    const double lowerBoundary = (node.currentSf - 7) * width;
    const double upperBoundary = (node.currentSf - 6) * width;
    int desiredSf = node.currentSf;
    if (distance < lowerBoundary - sfBoundaryHysteresis ||
            distance > upperBoundary + sfBoundaryHysteresis)
        desiredSf = 6 + clampInt((int)std::ceil(std::max(0.0, distance) / width), 1, 6);

    if (desiredSf == node.currentSf) {
        node.pendingSf = -1;
        node.pendingSfSamples = 0;
        return node.currentSf;
    }
    if (node.pendingSf == desiredSf)
        node.pendingSfSamples++;
    else {
        node.pendingSf = desiredSf;
        node.pendingSfSamples = 1;
    }
    if (node.pendingSfSamples < sfConfirmationSamples) {
        node.sfChangesBlockedByHysteresis++;
        return node.currentSf;
    }
    node.pendingSf = -1;
    node.pendingSfSamples = 0;
    return desiredSf;
}

void NetworkServerApp::evaluateAdaptiveAdrCommand(Packet *packet, KnownNode& node,
        const L3Address& gateway, double bestSnr, double bestRssi)
{
    const auto& frame = packet->peekAtFront<LoRaMacFrame>();
    std::unique_ptr<Packet> payloadView(packet->dup());
    payloadView->trimFront();
    payloadView->removeAtFront<LoRaMacFrame>();
    const auto& appPacket = payloadView->peekAtFront<LoRaAppPacket>();

    updateAdaptiveObservation(node, frame, appPacket, bestSnr, bestRssi);
    node.framesSinceAdr++;

    const int decisionWindow = node.mobilityClass == ADR_MOBILITY_STATIC ?
            staticAdrWindowSize : mobileAdrWindowSize;
    const bool adrAckRequested = appPacket->getOptions().getADRACKReq();
    const double desiredHeartbeat = node.mobilityClass == ADR_MOBILITY_STATIC &&
            node.deteriorationSamplesRemaining == 0 ? stableHeartbeatInterval : activeHeartbeatInterval;
    const bool controlChanged = desiredHeartbeat != node.commandedHeartbeatInterval ||
            node.mobilityClass != node.lastCommandedMobilityClass;

    if (node.framesSinceAdr < decisionWindow && !adrAckRequested && !controlChanged)
        return;

    int proposedSf = node.currentSf;
    int proposedTp = clampInt((int)std::round(node.currentPowerDbm), 2, 14);
    int proposedChannel = 0;
    for (size_t i = 1; i < eu868ChannelsMhz.size(); ++i)
        if (std::abs(eu868ChannelsMhz[i] - node.currentChannelMhz) <
                std::abs(eu868ChannelsMhz[proposedChannel] - node.currentChannelMhz))
            proposedChannel = i;

    if (node.framesSinceAdr >= decisionWindow || adrAckRequested) {
        node.framesSinceAdr = 0;
        const int payloadBytes = (packet->getDataLength().get() + 7) / 8;
        int fixedSf = -1;
        double safetyMargin = staticAdrMargin;
        if (node.mobilityClass != ADR_MOBILITY_STATIC) {
            if (constrainMobileSfByDistance)
                fixedSf = confirmedMobileSpreadingFactor(node,
                        std::max(0.0, appPacket->getOptions().getDistanceED()));
            safetyMargin = mobileAdrMargin;
        }
        KnownNode decisionState = node;
        if (node.mobilityClass == ADR_MOBILITY_STATIC && !node.snrHistory.empty()) {
            // Static mode deliberately filters over the full long window. In
            // mobile/unknown mode the short EWMA above remains the estimator.
            decisionState.ewmaSnr = adrMethod == "max" ?
                    *std::max_element(node.snrHistory.begin(), node.snrHistory.end()) :
                    std::accumulate(node.snrHistory.begin(), node.snrHistory.end(), 0.0) /
                            node.snrHistory.size();
        }
        AdrCandidate candidate = selectEnergyCandidate(decisionState, frame, payloadBytes,
                fixedSf, safetyMargin);

        // Static SF changes also need repeated evidence. Mobile SF evidence is
        // already handled against distance-band boundaries above.
        if ((node.mobilityClass == ADR_MOBILITY_STATIC || !constrainMobileSfByDistance) &&
                candidate.spreadingFactor != node.currentSf) {
            if (node.pendingSf == candidate.spreadingFactor)
                node.pendingSfSamples++;
            else {
                node.pendingSf = candidate.spreadingFactor;
                node.pendingSfSamples = 1;
            }
            if (node.pendingSfSamples < sfConfirmationSamples) {
                node.sfChangesBlockedByHysteresis++;
                candidate = selectEnergyCandidate(decisionState, frame, payloadBytes,
                        node.currentSf, safetyMargin);
            }
            else {
                node.pendingSf = -1;
                node.pendingSfSamples = 0;
            }
        }

        proposedSf = candidate.spreadingFactor;
        proposedChannel = candidate.channelIndex;
        const int rawTp = candidate.transmitPowerDbm;
        if (rawTp < proposedTp)
            proposedTp = std::max(rawTp, proposedTp - maxTpDecreasePerCommand);
        else if (rawTp > proposedTp)
            proposedTp = std::min(rawTp, proposedTp + maxTpIncreasePerCommand);
        node.lastPredictedSuccess = candidate.predictedSuccess;
        node.lastEnergyCost = candidate.energyCostJ;
    }

    sendAdrCommand(frame, node, gateway, proposedSf, proposedTp, proposedChannel,
            desiredHeartbeat, node.mobilityClass, adrAckRequested);
}

void NetworkServerApp::sendAdrCommand(const Ptr<const LoRaMacFrame>& frame, KnownNode& node,
        const L3Address& gateway, int sf, int tpDbm, int channelIndex,
        double heartbeatInterval, int mobilityClass, bool forceResponse, double feedbackSnr)
{
    const double channelMhz = eu868ChannelsMhz[channelIndex];
    const bool radioChanged = sf != node.currentSf || tpDbm != (int)std::round(node.currentPowerDbm) ||
            std::abs(channelMhz - node.currentChannelMhz) > 1e-6;
    const bool heartbeatChanged = heartbeatInterval >= 0 &&
            heartbeatInterval != node.commandedHeartbeatInterval;
    const bool mobilityChanged = mobilityClass >= 0 && mobilityClass != node.lastCommandedMobilityClass;
    if (!radioChanged && !heartbeatChanged && !mobilityChanged && !forceResponse) {
        node.suppressedNoOpCommands++;
        return;
    }

    LoRaOptions options;
    options.setLoRaSF(sf);
    options.setLoRaTP(tpDbm);
    options.setLoRaCF(channelMhz);
    options.setHeartbeatInterval(heartbeatInterval);
    options.setMobilityClass(mobilityClass);
    options.setFeedbackSnr(feedbackSnr);

    auto command = makeShared<LoRaAppPacket>();
    command->setMsgType(TXCONFIG);
    command->setOptions(options);
    command->setChunkLength(B(par("headerLength").intValue()));

    auto downlinkFrame = makeShared<LoRaMacFrame>();
    downlinkFrame->setChunkLength(B(par("headerLength").intValue()));
    downlinkFrame->setReceiverAddress(frame->getTransmitterAddress());
    downlinkFrame->setLoRaTP(math::dBmW2mW(14));
    downlinkFrame->setLoRaCF(frame->getLoRaCF());
    downlinkFrame->setLoRaSF(frame->getLoRaSF());
    downlinkFrame->setLoRaBW(frame->getLoRaBW());
    downlinkFrame->setLoRaCR(frame->getLoRaCR());

    auto downlink = new Packet("ADRPacket");
    downlink->insertAtFront(command);
    downlink->insertAtFront(downlinkFrame);
    socket.sendTo(downlink, gateway, destPort);

    if (sf != node.currentSf)
        node.sfChanges++;
    if (tpDbm > node.currentPowerDbm)
        node.tpIncreases++;
    else if (tpDbm < node.currentPowerDbm)
        node.tpDecreases++;
    if (std::abs(channelMhz - node.currentChannelMhz) > 1e-6)
        node.channelChanges++;
    node.currentSf = sf;
    node.currentPowerDbm = tpDbm;
    node.currentChannelMhz = channelMhz;
    if (heartbeatInterval >= 0)
        node.commandedHeartbeatInterval = heartbeatInterval;
    if (mobilityClass >= 0)
        node.lastCommandedMobilityClass = mobilityClass;
    if (simTime() >= getSimulation()->getWarmupPeriod())
        node.sentAdrCommands++;
}

void NetworkServerApp::sendConfirmedAck(const Ptr<const LoRaMacFrame>& frame,
        const Ptr<const LoRaAppPacket>& appPacket, const L3Address& gateway)
{
    auto ackPayload = makeShared<LoRaAppPacket>();
    ackPayload->setMsgType(ACK);
    ackPayload->setOptions(appPacket->getOptions());
    ackPayload->setChunkLength(B(par("headerLength").intValue()));

    auto ackFrame = makeShared<LoRaMacFrame>();
    ackFrame->setChunkLength(B(par("headerLength").intValue()));
    ackFrame->setReceiverAddress(frame->getTransmitterAddress());
    ackFrame->setSequenceNumber(frame->getSequenceNumber());
    ackFrame->setLoRaTP(math::dBmW2mW(14));
    ackFrame->setLoRaCF(frame->getLoRaCF());
    ackFrame->setLoRaSF(frame->getLoRaSF());
    ackFrame->setLoRaBW(frame->getLoRaBW());
    ackFrame->setLoRaCR(frame->getLoRaCR());

    auto ack = new Packet("ACKPacket");
    ack->insertAtFront(ackPayload);
    ack->insertAtFront(ackFrame);
    socket.sendTo(ack, gateway, destPort);
}

int NetworkServerApp::findLeastLoadedChannel() const
{
    const auto loads = channelLoads();
    return std::distance(loads.begin(), std::min_element(loads.begin(), loads.end()));
}

double NetworkServerApp::requiredSnr(int spreadingFactor)
{
    static const double thresholds[] = {-7.5, -10, -12.5, -15, -17.5, -20};
    if (spreadingFactor < 7 || spreadingFactor > 12)
        throw cRuntimeError("Invalid LoRa spreading factor %d", spreadingFactor);
    return thresholds[spreadingFactor - 7];
}

bool NetworkServerApp::isAccountingPacket(const Ptr<const LoRaMacFrame>& frame,
        const Ptr<const LoRaAppPacket>& appPacket)
{
    return !appPacket->getOptions().isHeartbeat() && !frame->isGeneratedByGateway();
}

void NetworkServerApp::receiveSignal(cComponent *, simsignal_t, intval_t spreadingFactor,
        cObject *)
{
    if (simTime() >= getSimulation()->getWarmupPeriod()) {
        sentDataPackets++;
        if (spreadingFactor >= 7 && spreadingFactor <= 12)
            sentDataPacketsPerSf[spreadingFactor - 7]++;
    }
}

void NetworkServerApp::finish()
{
    const long accountedDeliveries = std::min(uniqueDataPackets, sentDataPackets);
    const double der = sentDataPackets > 0 ? (double)accountedDeliveries / sentDataPackets : 0;
    recordScalar("LoRa_NS_DER", der);
    recordScalar("counterOfSentPacketsFromNodes", sentDataPackets);
    recordScalar("counterUniqueReceivedPackets", accountedDeliveries);
    recordScalar("rawUniqueReceivedPackets", uniqueDataPackets);
    recordScalar("totalReceivedPackets", accountedDeliveries);
    recordScalar("totalReceivedCopies", totalReceivedCopies);
    recordScalar("heartbeatCopies", heartbeatCopies);
    recordScalar("csiNotifications", csiNotifications);
    recordScalar("adrAlgorithmCode", static_cast<int>(adrAlgorithm));
    recordScalar("adrStepSizeDb", adrStepSize, "dB");
    recordScalar("useCsiNotificationsForAdr", useCsiNotificationsForAdr);
    recordScalar("collisionModelScale", collisionModelScale);
    recordScalar("constrainMobileSfByDistance", constrainMobileSfByDistance);
    recordScalar("enableM1", enableM1);
    recordScalar("enableM2", enableM2);
    recordScalar("forcePeriodicAdrResponse", forcePeriodicAdrResponse);

    for (int sf = 7; sf <= 12; ++sf) {
        const int index = sf - 7;
        recordScalar(("counterUniqueReceivedPacketsPerSF SF" + std::to_string(sf)).c_str(),
                uniqueDataPacketsPerSf[index]);
        const double sfDer = sentDataPacketsPerSf[index] > 0 ?
                (double)uniqueDataPacketsPerSf[index] / sentDataPacketsPerSf[index] : 0;
        recordScalar(("DER SF" + std::to_string(sf)).c_str(), sfDer);
    }
    for (size_t channel = 0; channel < eu868ChannelsMhz.size(); ++channel) {
        const std::string suffix = std::to_string(channel);
        recordScalar(("uniqueReceivedChannel" + suffix).c_str(),
                uniqueDataPacketsPerChannel[channel]);
    }

    for (size_t nodeIndex = 0; nodeIndex < knownNodes.size(); ++nodeIndex) {
        const auto& node = knownNodes[nodeIndex];
        recordScalar("Send ADR for node", node.sentAdrCommands);
        recordScalar("SF distribution", node.currentSf);
        // Use the server's stable discovery index in scalar names; formatting
        // a 48-bit MAC as a signed int produced unreadable pseudo node IDs.
        const std::string id = std::to_string(nodeIndex);
        recordScalar(("adaptiveMobilityClass node " + id).c_str(), node.mobilityClass);
        recordScalar(("adaptiveEwmaSpeed node " + id).c_str(), node.ewmaSpeed, "mps");
        recordScalar(("adaptiveEwmaSNR node " + id).c_str(), node.ewmaSnr, "dB");
        recordScalar(("adaptiveEwmaRSSI node " + id).c_str(), node.ewmaRssi, "dBm");
        recordScalar(("adaptiveEstimatedPDR node " + id).c_str(), node.ewmaDelivery);
        recordScalar(("adaptivePredictedSuccess node " + id).c_str(), node.lastPredictedSuccess);
        recordScalar(("adaptiveEnergyCost node " + id).c_str(), node.lastEnergyCost, "J");
        recordScalar(("adaptiveHeartbeatInterval node " + id).c_str(),
                node.commandedHeartbeatInterval, "s");
        recordScalar(("adaptiveNoOpSuppressed node " + id).c_str(), node.suppressedNoOpCommands);
        recordScalar(("adaptiveMobilityTransitions node " + id).c_str(), node.mobilityTransitions);
        recordScalar(("adaptiveDeteriorationEvents node " + id).c_str(), node.deteriorationEvents);
        recordScalar(("adaptiveSFChanges node " + id).c_str(), node.sfChanges);
        recordScalar(("adaptiveSFChangesBlocked node " + id).c_str(), node.sfChangesBlockedByHysteresis);
        recordScalar(("adaptiveTPIncreases node " + id).c_str(), node.tpIncreases);
        recordScalar(("adaptiveTPDecreases node " + id).c_str(), node.tpDecreases);
        recordScalar(("adaptiveChannelChanges node " + id).c_str(), node.channelChanges);
        recordScalar(("ndAdrWindow node " + id).c_str(), node.ndWindow);
        recordScalar(("ndAdrDecisions node " + id).c_str(), node.ndDecisions);
    }
    for (const auto& entry : receivedPerNode) {
        const int nodeId = entry.first;
        const int received = entry.second;
        recordScalar(("numReceivedFromNode " + std::to_string(nodeId)).c_str(), received);
        const int sent = sentPerNode[nodeId];
        recordScalar(("numSentPerED " + std::to_string(nodeId)).c_str(), sent);
        recordScalar(("DERperNode " + std::to_string(nodeId)).c_str(),
                sent > 0 ? (double)received / sent : 0);
        recordScalar(("SNRperED " + std::to_string(nodeId)).c_str(), lastSnrPerNode[nodeId]);
        recordScalar(("RSSIperED " + std::to_string(nodeId)).c_str(), lastRssiPerNode[nodeId]);
    }
    receivedRssi.recordAs("receivedRSSI");
}

void NetworkServerApp::clearPendingUplinks()
{
    for (auto& pending : pendingUplinks) {
        if (pending.waitingTimer != nullptr) {
            if (pending.waitingTimer->isScheduled())
                cancelEvent(pending.waitingTimer);
            delete pending.waitingTimer;
        }
        delete pending.packet;
    }
    pendingUplinks.clear();
}

} // namespace flora
