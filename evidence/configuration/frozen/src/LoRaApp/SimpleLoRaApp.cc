//
// Copyright (C) 2026
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include "SimpleLoRaApp.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "inet/common/INETMath.h"
#include "inet/common/packet/Packet.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/mobility/static/StationaryMobility.h"

#include "LoRa/LoRaTagInfo_m.h"

namespace flora {

Define_Module(SimpleLoRaApp);

SimpleLoRaApp::~SimpleLoRaApp()
{
    cancelAndDelete(sendMeasurements);
    cancelAndDelete(heartbeatTimer);
}

void SimpleLoRaApp::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        packetSentSignal = registerSignal("LoRa_AppPacketSent");
        sfChangesSignal = registerSignal("CambiosSF");
        tpChangesSignal = registerSignal("CambiosTP");
        heartbeatIntervalChangedSignal = registerSignal("HeartbeatIntervalChanged");
        mobilityClassChangedSignal = registerSignal("MobilityClassChanged");

        auto host = getContainingNode(this);
        gatewayX = host->par("gatewayX").doubleValue();
        gatewayY = host->par("gatewayY").doubleValue();

        // The thesis static testbed uses a uniform disk. A mobile node lets
        // RandomWaypointMobility choose its initial position and waypoints.
        if (!host->par("mobile").boolValue()
                && !strcmp(host->par("deploymentType").stringValue(), "circle")) {
            const auto coordinates = generateUniformCircleCoordinates(
                    host->par("maxGatewayDistance").doubleValue(), gatewayX, gatewayY);
            auto mobility = check_and_cast<StationaryMobility *>(host->getSubmodule("mobility"));
            mobility->par("initialX").setDoubleValue(coordinates.first);
            mobility->par("initialY").setDoubleValue(coordinates.second);
        }
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        auto nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
        if (nodeStatus && nodeStatus->getState() != NodeStatus::UP)
            throw cRuntimeError("SimpleLoRaApp cannot start while its node is down");

        numberOfPacketsToSend = par("numberOfPacketsToSend");
        timeToFirstPacket = par("timeToFirstPacket");
        heartbeatInterval = par("heartbeatInterval");
        heartbeatOnlyWhenDataSilent = par("heartbeatOnlyWhenDataSilent");
        evaluateADRinNode = par("evaluateADRinNode");
        includePosition = par("includePosition");
        positionPayloadBytes = par("positionPayloadBytes");
        localizationEnergyPerFixJ = par("localizationEnergyPerFix").doubleValue();
        clientAdrMode = par("clientAdrMode").stdstringValue();
        if (clientAdrMode != "network" && clientAdrMode != "blind" &&
                clientAdrMode != "hadrMobile" && clientAdrMode != "madere")
            throw cRuntimeError("clientAdrMode must be network, blind, hadrMobile or madere");
        madereCellSizeM = par("madereCellSize").doubleValue();
        madereSnrMarginCells = par("madereSnrMarginCells");
        madereMaxSpeedMps = par("madereMaxSpeed").doubleValue();
        madereGamma = par("madereGamma");
        madereD0M = par("madereD0").doubleValue();
        madereNoiseFigureDb = par("madereNoiseFigure");
        madereFrequencyHz = par("madereFrequency").doubleValue();
        madereMaxDistanceM = getContainingNode(this)->par("maxGatewayDistance").doubleValue();
        if (madereCellSizeM <= 0 || madereSnrMarginCells < 0 || madereMaxSpeedMps < 0 ||
                madereGamma <= 0 || madereD0M <= 0 || madereMaxDistanceM <= 0)
            throw cRuntimeError("Invalid MADERE radial-model parameter");
        if (positionPayloadBytes < 0 || localizationEnergyPerFixJ < 0)
            throw cRuntimeError("Position bytes and localization energy must be non-negative");
        initialPosition = getCurrentPosition();

        loRaBW = Hz(par("initialLoRaBW").doubleValue());
        loRaCR = par("initialLoRaCR");
        loRaUseHeader = par("initialUseHeader");

        if (par("randomizeInitialRadioParameters").boolValue()) {
            static constexpr std::array<int, 7> allowedPowers = {2, 4, 6, 8, 10, 12, 14};
            loRaTP = allowedPowers[intuniform(0, allowedPowers.size() - 1)];
            loRaSF = intuniform(7, 12);
        }
        else {
            loRaTP = par("initialLoRaTP").doubleValue();
            loRaSF = par("initialLoRaSF");
        }

        // Legacy LoRaWAN uses the three mandatory EU868 channels. The thesis
        // proposal enables the five additional 125-kHz channels as well.
        std::vector<double> channelsMHz = {868.1, 868.3, 868.5};
        if (par("useAllEu868Channels").boolValue())
            channelsMHz.insert(channelsMHz.end(), {867.1, 867.3, 867.5, 867.7, 867.9});
        loRaCF = Hz(channelsMHz[intuniform(0, channelsMHz.size() - 1)] * 1e6);

        initialLoRaTP = loRaTP;
        initialLoRaSF = loRaSF;
        radioStateLastUpdate = simTime();
        if (clientAdrMode == "madere")
            initializeMadereRadialModel();
        sfVector.setName("SF Vector");
        tpVector.setName("TP Vector");

        sendMeasurements = new cMessage("sendMeasurements");
        scheduleAt(simTime() + timeToFirstPacket, sendMeasurements);

        if (heartbeatInterval > SIMTIME_ZERO) {
            heartbeatTimer = new cMessage("heartbeatTimer");
            scheduleAt(simTime() + heartbeatInterval, heartbeatTimer);
        }
    }
}

std::pair<double, double> SimpleLoRaApp::generateUniformCircleCoordinates(
        double radius, double centerX, double centerY)
{
    const double radialDistance = sqrt(uniform(0, radius * radius));
    const double angle = uniform(0, 2 * M_PI);
    return {centerX + radialDistance * cos(angle), centerY - radialDistance * sin(angle)};
}

Coord SimpleLoRaApp::getCurrentPosition() const
{
    auto host = getContainingNode(this);
    auto mobility = check_and_cast<IMobility *>(host->getSubmodule("mobility"));
    return mobility->getCurrentPosition();
}

double SimpleLoRaApp::getDistanceToGateway() const
{
    const Coord position = getCurrentPosition();
    return hypot(position.x - gatewayX, position.y - gatewayY);
}

void SimpleLoRaApp::handleMessage(cMessage *message)
{
    if (message == sendMeasurements) {
        sendDataPacket();
        if (numberOfPacketsToSend == 0 || sentPackets < numberOfPacketsToSend) {
            timeToNextPacket = par("timeToNextPacket");
            scheduleAt(simTime() + timeToNextPacket, sendMeasurements);
        }
    }
    else if (message == heartbeatTimer) {
        sendHeartbeat();
        if (heartbeatInterval > SIMTIME_ZERO)
            scheduleAt(simTime() + heartbeatInterval, heartbeatTimer);
    }
    else {
        handleDownlink(message);
        delete message;
    }
}

void SimpleLoRaApp::sendDataPacket()
{
    applyClientAdrForNextPacket();
    const int transmittedSf = loRaSF;
    sendUplink(false);
    restartHeartbeatAfterData();

    if (simTime() >= getSimulation()->getWarmupPeriod()) {
        sentPackets++;
        sentPacketCounter++;
        updateTrafficScheduleHash();
    }

    if (evaluateADRinNode) {
        adrAckCounter++;
        if (adrAckCounter == ADR_ACK_LIMIT)
            sendNextPacketWithADRACKReq = true;
        if (adrAckCounter >= ADR_ACK_LIMIT + ADR_ACK_DELAY) {
            adrAckCounter = 0;
            applyAdrRecoveryStep();
            numADRACKReq++;
        }
    }

    emit(packetSentSignal, transmittedSf);
    clientAdrPacketIndex++;
}

void SimpleLoRaApp::sendHeartbeat()
{
    numHeartbeats++;
    sendUplink(true);
}

void SimpleLoRaApp::sendUplink(bool heartbeat)
{
    auto packet = new Packet(heartbeat ? "Heartbeat" : "DataFrame");
    packet->setKind(DATA);

    auto payload = makeShared<LoRaAppPacket>();
    const int positionBytes = includePosition ? positionPayloadBytes : 0;
    payload->setChunkLength(B(par("dataSize").intValue() + positionBytes));
    payload->setSampleMeasurement(intuniform(0, INT_MAX));

    auto options = payload->getOptions();
    const Coord position = getCurrentPosition();
    options.setIsHeartbeat(heartbeat);
    options.setPacketCounter(sentPacketCounter);
    options.setDistanceED(includePosition ? getDistanceToGateway() : 0);
    options.setPositionX(includePosition ? position.x : 0);
    options.setPositionY(includePosition ? position.y : 0);
    options.setHasPosition(includePosition);
    options.setSentAt(simTime());
    if (!heartbeat && evaluateADRinNode && sendNextPacketWithADRACKReq) {
        options.setADRACKReq(true);
        sendNextPacketWithADRACKReq = false;
    }
    payload->setOptions(options);

    auto tag = packet->addTagIfAbsent<LoRaTag>();
    tag->setBandwidth(loRaBW);
    tag->setCenterFrequency(loRaCF);
    tag->setCodeRendundance(loRaCR);
    tag->setSpreadFactor(loRaSF);
    tag->setPower(mW(math::dBmW2mW(loRaTP)));
    tag->setIsHeartbeat(heartbeat);

    sfVector.record(loRaSF);
    tpVector.record(loRaTP);
    if (!heartbeat && simTime() >= getSimulation()->getWarmupPeriod()) {
        if (loRaSF >= 7 && loRaSF <= 12)
            uplinksPerSf[loRaSF - 7]++;
        const int index = channelIndex(loRaCF);
        if (index >= 0)
            uplinksPerChannel[index]++;
    }
    if (includePosition) {
        totalLocationFixes++;
        if (simTime() >= getSimulation()->getWarmupPeriod()) {
            measurementLocationFixes++;
            measurementPositionBytes += positionPayloadBytes;
        }
    }
    packet->insertAtBack(payload);
    send(packet, "appOut");
}

void SimpleLoRaApp::handleDownlink(cMessage *message)
{
    auto packet = check_and_cast<Packet *>(message);
    const auto& command = packet->peekAtFront<LoRaAppPacket>();

    if (simTime() >= getSimulation()->getWarmupPeriod())
        receivedADRCommands++;
    if (!evaluateADRinNode || command->getMsgType() != TXCONFIG)
        return;

    adrAckCounter = 0;
    const auto& options = command->getOptions();
    const double uplinkPowerDbm = loRaTP;

    if (options.getLoRaTP() != -1 && options.getLoRaTP() != loRaTP) {
        loRaTP = options.getLoRaTP();
        tpChanges++;
        emit(tpChangesSignal, tpChanges);
    }
    // The recovered MADERE ED ignores the conventional server SF and derives
    // its SF from the SNR-conditioned position distribution. It still applies
    // server TP, so do not count an instantaneous SF that is never transmitted.
    if (clientAdrMode != "madere" && options.getLoRaSF() != -1 &&
            options.getLoRaSF() != loRaSF) {
        changeSpreadingFactor(options.getLoRaSF());
        sfChanges++;
        emit(sfChangesSignal, sfChanges);
    }
    if (options.getLoRaCF() != -1 &&
            std::abs(options.getLoRaCF() * 1e6 - loRaCF.get()) > 0.5) {
        accountRadioStateUntil(simTime());
        loRaCF = Hz(options.getLoRaCF() * 1e6);
    }

    if (options.getHeartbeatInterval() >= 0) {
        const simtime_t requestedInterval = options.getHeartbeatInterval();
        if (requestedInterval != heartbeatInterval)
            applyHeartbeatInterval(requestedInterval);
    }
    if (options.getMobilityClass() >= ADR_MOBILITY_UNKNOWN &&
            options.getMobilityClass() <= ADR_MOBILITY_MOBILE &&
            options.getMobilityClass() != adrMobilityClass) {
        adrMobilityClass = options.getMobilityClass();
        mobilityClassChanges++;
        emit(mobilityClassChangedSignal, adrMobilityClass);
    }
    if (clientAdrMode == "madere" && options.getFeedbackSnr() > -998)
        updateMadereFromFeedback(options.getFeedbackSnr(), uplinkPowerDbm);
}

void SimpleLoRaApp::applyHeartbeatInterval(simtime_t newInterval)
{
    // Reschedule from the command reception time. This makes a short interval
    // react immediately to movement/deterioration and a long interval avoid an
    // already queued unnecessary heartbeat.
    if (heartbeatTimer->isScheduled())
        cancelEvent(heartbeatTimer);
    heartbeatInterval = newInterval;
    heartbeatIntervalChanges++;
    emit(heartbeatIntervalChangedSignal, heartbeatInterval);
    if (heartbeatInterval > SIMTIME_ZERO)
        scheduleAt(simTime() + heartbeatInterval, heartbeatTimer);
}

void SimpleLoRaApp::restartHeartbeatAfterData()
{
    // In the mobility-energy treatment a heartbeat is a keepalive, not a
    // second periodic traffic stream. A normal uplink already proves liveness,
    // so restart the current adaptive interval and transmit a heartbeat only
    // when the application remains silent for that whole interval. The switch
    // is false in the reference and thesis treatments to preserve their
    // published controls exactly.
    if (!heartbeatOnlyWhenDataSilent || heartbeatInterval <= SIMTIME_ZERO || !heartbeatTimer)
        return;
    if (heartbeatTimer->isScheduled())
        cancelEvent(heartbeatTimer);
    scheduleAt(simTime() + heartbeatInterval, heartbeatTimer);
    heartbeatResetsByData++;
}

void SimpleLoRaApp::applyAdrRecoveryStep()
{
    // LoRaWAN reconnection first restores maximum TP. Subsequent
    // ADR_ACK_DELAY cycles reduce the data rate one SF step at a time.
    if (loRaTP < 14)
        loRaTP = 14;
    else if (loRaSF < 12)
        changeSpreadingFactor(loRaSF + 1);
}

int SimpleLoRaApp::channelIndex(Hz frequency) const
{
    static constexpr std::array<double, 8> channelsHz = {
        868.1e6, 868.3e6, 868.5e6, 867.1e6, 867.3e6, 867.5e6, 867.7e6, 867.9e6
    };
    int closest = 0;
    for (size_t i = 1; i < channelsHz.size(); ++i)
        if (std::abs(frequency.get() - channelsHz[i]) <
                std::abs(frequency.get() - channelsHz[closest]))
            closest = i;
    return closest;
}

void SimpleLoRaApp::accountRadioStateUntil(simtime_t now)
{
    const simtime_t start = std::max(radioStateLastUpdate,
            getSimulation()->getWarmupPeriod());
    if (now > start) {
        const double seconds = (now - start).dbl();
        if (loRaSF >= 7 && loRaSF <= 12)
            sfResidenceSeconds[loRaSF - 7] += seconds;
        const int index = channelIndex(loRaCF);
        if (index >= 0)
            channelResidenceSeconds[index] += seconds;
    }
    radioStateLastUpdate = now;
}

void SimpleLoRaApp::changeSpreadingFactor(int newSf)
{
    if (newSf == loRaSF)
        return;
    if (newSf < 7 || newSf > 12)
        throw cRuntimeError("Invalid spreading factor %d", newSf);
    accountRadioStateUntil(simTime());
    if (loRaSF >= 7 && loRaSF <= 12)
        sfTransitions[loRaSF - 7][newSf - 7]++;
    loRaSF = newSf;
}

void SimpleLoRaApp::updateTrafficScheduleHash()
{
    const uint64_t timestamp = static_cast<uint64_t>(simTime().inUnit(SIMTIME_US));
    trafficScheduleHash ^= timestamp;
    trafficScheduleHash *= 1099511628211ULL;
}

void SimpleLoRaApp::applyClientAdrForNextPacket()
{
    if (clientAdrMode == "network" || clientAdrMode == "madere")
        return;
    // Recovered Blind ADR hour: one SF12, two SF10 and three SF7 uplinks.
    // Mapping that sequence onto common application events preserves its SF
    // exposure without the historical implementation's second traffic stream.
    static constexpr std::array<int, 6> blindSequence = {12, 10, 10, 7, 7, 7};
    // Recovered mobile HADR/BADR branch cycles once through SF7..SF12.
    static constexpr std::array<int, 6> hadrSequence = {7, 8, 9, 10, 11, 12};
    const auto& sequence = clientAdrMode == "blind" ? blindSequence : hadrSequence;
    const int nextSf = sequence[clientAdrPacketIndex % sequence.size()];
    if (nextSf != loRaSF) {
        changeSpreadingFactor(nextSf);
        sfChanges++;
        emit(sfChangesSignal, sfChanges);
    }
}

void SimpleLoRaApp::initializeMadereRadialModel()
{
    const int bins = std::max(1, (int)std::ceil(madereMaxDistanceM / madereCellSizeM));
    madereRadialProbability.assign(bins, 0.0);
    const int initialBin = std::min(bins - 1,
            std::max(0, (int)(getDistanceToGateway() / madereCellSizeM)));
    madereRadialProbability[initialBin] = 1.0;
    madereEstimatedDistanceM = (initialBin + 0.5) * madereCellSizeM;
    madereLastUpdate = simTime();
}

void SimpleLoRaApp::propagateMadereProbability(double distanceM)
{
    const int radius = std::max(0, (int)std::ceil(distanceM / madereCellSizeM));
    if (radius == 0 || madereRadialProbability.empty())
        return;
    std::vector<double> difference(madereRadialProbability.size() + 1, 0.0);
    for (size_t source = 0; source < madereRadialProbability.size(); ++source) {
        if (madereRadialProbability[source] <= 0)
            continue;
        const int first = std::max(0, (int)source - radius);
        const int last = std::min((int)madereRadialProbability.size() - 1,
                (int)source + radius);
        const double share = madereRadialProbability[source] / (last - first + 1);
        difference[first] += share;
        difference[last + 1] -= share;
    }
    double running = 0;
    for (size_t i = 0; i < madereRadialProbability.size(); ++i) {
        running += difference[i];
        madereRadialProbability[i] = running;
    }
}

void SimpleLoRaApp::updateMadereFromFeedback(double snr, double uplinkPowerDbm)
{
    if (madereRadialProbability.empty())
        initializeMadereRadialModel();
    const double noiseFloorDbm = -174.0 + 10.0 * std::log10(loRaBW.get()) +
            madereNoiseFigureDb;
    const double wavelength = 3e8 / madereFrequencyHz;
    const double pathLossD0Db = 20.0 * std::log10(4.0 * M_PI * madereD0M / wavelength);
    const double pathLossDb = uplinkPowerDbm - snr - noiseFloorDbm;
    double observedDistance = madereD0M * std::pow(10.0,
            (pathLossDb - pathLossD0Db) / (10.0 * madereGamma));
    observedDistance = std::max(0.0, std::min(madereMaxDistanceM, observedDistance));
    const double halfWidth = madereSnrMarginCells * madereCellSizeM;

    double retained = 0;
    int candidates = 0;
    for (size_t i = 0; i < madereRadialProbability.size(); ++i) {
        const double distance = (i + 0.5) * madereCellSizeM;
        if (std::abs(distance - observedDistance) <= halfWidth) {
            retained += madereRadialProbability[i];
            candidates++;
        }
        else
            madereRadialProbability[i] = 0;
    }
    if (retained <= 0 && candidates > 0) {
        for (size_t i = 0; i < madereRadialProbability.size(); ++i) {
            const double distance = (i + 0.5) * madereCellSizeM;
            if (std::abs(distance - observedDistance) <= halfWidth)
                madereRadialProbability[i] = 1.0 / candidates;
        }
    }
    else if (retained > 0) {
        for (double& probability : madereRadialProbability)
            probability /= retained;
    }

    const double elapsed = std::max(0.0, (simTime() - madereLastUpdate).dbl());
    propagateMadereProbability(madereMaxSpeedMps * elapsed);
    madereLastUpdate = simTime();

    double probabilitySum = 0;
    double weightedDistance = 0;
    for (size_t i = 0; i < madereRadialProbability.size(); ++i) {
        probabilitySum += madereRadialProbability[i];
        weightedDistance += (i + 0.5) * madereCellSizeM * madereRadialProbability[i];
    }
    madereEstimatedDistanceM = probabilitySum > 0 ? weightedDistance / probabilitySum :
            observedDistance;
    int newSf = 12;
    if (madereEstimatedDistanceM < 2500) newSf = 7;
    else if (madereEstimatedDistanceM < 5000) newSf = 8;
    else if (madereEstimatedDistanceM < 7500) newSf = 9;
    else if (madereEstimatedDistanceM < 10000) newSf = 10;
    else if (madereEstimatedDistanceM < 12500) newSf = 11;
    if (newSf != loRaSF) {
        changeSpreadingFactor(newSf);
        sfChanges++;
        emit(sfChangesSignal, sfChanges);
    }
    madereFeedbacks++;
}

void SimpleLoRaApp::finish()
{
    accountRadioStateUntil(simTime());
    const Coord position = getCurrentPosition();
    recordScalar("initialPositionX", initialPosition.x);
    recordScalar("initialPositionY", initialPosition.y);
    recordScalar("positionX", position.x);
    recordScalar("positionY", position.y);
    recordScalar("displacement", position.distance(initialPosition), "m");
    recordScalar("distanceToGateway", getDistanceToGateway());
    recordScalar("initialLoRaTP", initialLoRaTP);
    recordScalar("initialLoRaSF", initialLoRaSF);
    recordScalar("finalTP", loRaTP);
    recordScalar("finalSF", loRaSF);
    recordScalar("finalCF", loRaCF.get() / 1e6);
    recordScalar("sentPackets", sentPackets);
    recordScalar("receivedADRCommands", receivedADRCommands);
    recordScalar("ADRRecoverySteps", numADRACKReq);
    recordScalar("heartbeats", numHeartbeats);
    recordScalar("finalHeartbeatInterval", heartbeatInterval.dbl(), "s");
    recordScalar("heartbeatIntervalChanges", heartbeatIntervalChanges);
    recordScalar("heartbeatResetsByData", heartbeatResetsByData);
    recordScalar("finalAdrMobilityClass", adrMobilityClass);
    recordScalar("mobilityClassChanges", mobilityClassChanges);
    recordScalar("SFChanges", sfChanges);
    recordScalar("TPChanges", tpChanges);
    recordScalar("includePosition", includePosition);
    recordScalar("positionPayloadBytes", positionPayloadBytes, "B");
    recordScalar("totalLocationFixes", totalLocationFixes);
    recordScalar("measurementLocationFixes", measurementLocationFixes);
    recordScalar("measurementPositionBytes", measurementPositionBytes, "B");
    recordScalar("measurementLocationEnergyConsumed",
            measurementLocationFixes * localizationEnergyPerFixJ, "J");
    recordScalar("trafficScheduleHashLow",
            static_cast<uint32_t>(trafficScheduleHash & 0xffffffffULL));
    recordScalar("trafficScheduleHashHigh",
            static_cast<uint32_t>(trafficScheduleHash >> 32));
    recordScalar("clientAdrModeCode", clientAdrMode == "network" ? 0 :
            (clientAdrMode == "blind" ? 1 : (clientAdrMode == "hadrMobile" ? 2 : 3)));
    recordScalar("madereFeedbacks", madereFeedbacks);
    recordScalar("madereEstimatedDistance", madereEstimatedDistanceM, "m");
    for (int sf = 7; sf <= 12; ++sf) {
        const std::string suffix = std::to_string(sf);
        recordScalar(("sfResidenceSeconds SF" + suffix).c_str(),
                sfResidenceSeconds[sf - 7], "s");
        recordScalar(("uplinksPerSF SF" + suffix).c_str(), uplinksPerSf[sf - 7]);
        for (int destination = 7; destination <= 12; ++destination)
            recordScalar(("sfTransition " + suffix + " to " +
                    std::to_string(destination)).c_str(),
                    sfTransitions[sf - 7][destination - 7]);
    }
    for (size_t channel = 0; channel < channelResidenceSeconds.size(); ++channel) {
        const std::string suffix = std::to_string(channel);
        recordScalar(("channelResidenceSeconds " + suffix).c_str(),
                channelResidenceSeconds[channel], "s");
        recordScalar(("uplinksPerChannel " + suffix).c_str(), uplinksPerChannel[channel]);
    }
}

bool SimpleLoRaApp::handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
}

} // namespace flora
