//
// Copyright (C) 2026
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#ifndef __FLORA_SIMPLELORAAPP_H
#define __FLORA_SIMPLELORAAPP_H

#include <omnetpp.h>

#include <array>
#include <cstdint>
#include <vector>

#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/geometry/common/Coord.h"

#include "LoRaAppPacket_m.h"

using namespace omnetpp;
using namespace inet;

namespace flora {

/**
 * Periodic LoRa application used by the thesis testbed.
 *
 * The application implements the end-device side of ADR recovery, periodic
 * heartbeat messages, EU868 channel selection, and position feedback for the
 * distance-based ADR strategy at the network server.
 */
class SimpleLoRaApp : public cSimpleModule, public ILifecycle
{
  protected:
    cMessage *sendMeasurements = nullptr;
    cMessage *heartbeatTimer = nullptr;

    int numberOfPacketsToSend = 0;
    int sentPackets = 0;
    int sentPacketCounter = 0;
    int receivedADRCommands = 0;
    int numADRACKReq = 0;
    int numHeartbeats = 0;
    int heartbeatIntervalChanges = 0;
    int heartbeatResetsByData = 0;
    int mobilityClassChanges = 0;

    simtime_t timeToFirstPacket;
    simtime_t timeToNextPacket;
    simtime_t heartbeatInterval;
    int adrMobilityClass = ADR_MOBILITY_UNKNOWN;

    bool evaluateADRinNode = false;
    bool heartbeatOnlyWhenDataSilent = false;
    bool sendNextPacketWithADRACKReq = false;
    int adrAckCounter = 0;
    static constexpr int ADR_ACK_LIMIT = 64;
    static constexpr int ADR_ACK_DELAY = 32;

    Hz loRaCF = Hz(NaN);
    Hz loRaBW = Hz(NaN);
    double loRaTP = NaN;
    int loRaSF = -1;
    int loRaCR = -1;
    bool loRaUseHeader = true;

    double initialLoRaTP = NaN;
    int initialLoRaSF = -1;
    double gatewayX = NaN;
    double gatewayY = NaN;
    Coord initialPosition;

    cOutVector sfVector;
    cOutVector tpVector;
    simsignal_t packetSentSignal;
    simsignal_t sfChangesSignal;
    simsignal_t tpChangesSignal;
    simsignal_t heartbeatIntervalChangedSignal;
    simsignal_t mobilityClassChangedSignal;
    int sfChanges = 0;
    int tpChanges = 0;
    bool includePosition = false;
    int positionPayloadBytes = 0;
    double localizationEnergyPerFixJ = 0;
    long totalLocationFixes = 0;
    long measurementLocationFixes = 0;
    long measurementPositionBytes = 0;
    std::array<double, 6> sfResidenceSeconds = {};
    std::array<double, 8> channelResidenceSeconds = {};
    std::array<long, 6> uplinksPerSf = {};
    std::array<long, 8> uplinksPerChannel = {};
    std::array<std::array<long, 6>, 6> sfTransitions = {};
    simtime_t radioStateLastUpdate = SIMTIME_ZERO;
    uint64_t trafficScheduleHash = 1469598103934665603ULL;
    std::string clientAdrMode = "network";
    long clientAdrPacketIndex = 0;
    std::vector<double> madereRadialProbability;
    double madereCellSizeM = 300;
    double madereSnrMarginCells = 8;
    double madereMaxSpeedMps = 3;
    double madereGamma = 2.32;
    double madereD0M = 1000;
    double madereNoiseFigureDb = 6;
    double madereFrequencyHz = 868e6;
    double madereMaxDistanceM = 15000;
    simtime_t madereLastUpdate = SIMTIME_ZERO;
    long madereFeedbacks = 0;
    double madereEstimatedDistanceM = 0;

  protected:
    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *message) override;
    virtual bool handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback) override;

    std::pair<double, double> generateUniformCircleCoordinates(double radius, double centerX, double centerY);
    Coord getCurrentPosition() const;
    double getDistanceToGateway() const;

    void handleDownlink(cMessage *message);
    void sendDataPacket();
    void sendHeartbeat();
    void sendUplink(bool heartbeat);
    void applyAdrRecoveryStep();
    void applyHeartbeatInterval(simtime_t newInterval);
    void restartHeartbeatAfterData();
    void accountRadioStateUntil(simtime_t now);
    int channelIndex(Hz frequency) const;
    void changeSpreadingFactor(int newSf);
    void updateTrafficScheduleHash();
    void applyClientAdrForNextPacket();
    void initializeMadereRadialModel();
    void updateMadereFromFeedback(double snr, double uplinkPowerDbm);
    void propagateMadereProbability(double distanceM);

  public:
    SimpleLoRaApp() = default;
    virtual ~SimpleLoRaApp();
    Hz getLoRaCF() const { return loRaCF; }
    Hz getLoRaBW() const { return loRaBW; }
    int getLoRaSF() const { return loRaSF; }
};

} // namespace flora

#endif
