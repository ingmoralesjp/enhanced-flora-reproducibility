#ifndef __FLORA_NETWORKSERVERAPP_H
#define __FLORA_NETWORKSERVERAPP_H

#include <deque>
#include <map>
#include <tuple>
#include <vector>

#include <omnetpp.h>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/INETDefs.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include "LoRaMacFrame_m.h"
#include "../LoRaApp/LoRaAppPacket_m.h"

namespace flora {

using namespace inet;
using namespace omnetpp;

struct KnownNode
{
    MacAddress address;
    int lastSequenceNumber = -1;
    int lastProcessedSequenceNumber = -1;
    int lastAdaptiveSequenceNumber = -1;
    int framesSinceAdr = 0;
    int sentAdrCommands = 0;
    int currentSf = 12;
    double currentPowerDbm = 14;
    double currentChannelMhz = 868.1;
    std::deque<double> snrHistory;

    // Mobility classification is temporal: one displacement sample cannot
    // switch mode. Separate evidence counters provide asymmetric hysteresis.
    int mobilityClass = ADR_MOBILITY_UNKNOWN;
    int lastCommandedMobilityClass = ADR_MOBILITY_UNKNOWN;
    bool hasPosition = false;
    double lastPositionX = 0;
    double lastPositionY = 0;
    simtime_t lastObservationTime = SIMTIME_ZERO;
    double ewmaSpeed = 0;
    int movingEvidence = 0;
    int stationaryEvidence = 0;
    int mobilityTransitions = 0;

    // Short EWMAs react to deterioration; the bounded SNR history remains
    // available for the longer, stable-node decision window.
    bool linkEwmaInitialized = false;
    double ewmaSnr = 0;
    double ewmaRssi = 0;
    double ewmaDelivery = 1;
    int deteriorationEvidence = 0;
    int deteriorationSamplesRemaining = 0;
    int deteriorationEvents = 0;

    // A new SF must remain outside the current distance band for several
    // observations before it becomes eligible.
    int pendingSf = -1;
    int pendingSfSamples = 0;

    double commandedHeartbeatInterval = -1;
    double lastPredictedSuccess = 0;
    double lastEnergyCost = 0;
    int suppressedNoOpCommands = 0;
    int sfChanges = 0;
    int sfChangesBlockedByHysteresis = 0;
    int tpIncreases = 0;
    int tpDecreases = 0;
    int channelChanges = 0;
    std::deque<double> ndSnrHistory;
    int ndWindow = 3;
    int ndDecisions = 0;
};

struct PendingUplink
{
    Packet *packet = nullptr;
    cMessage *waitingTimer = nullptr;
    std::vector<std::tuple<L3Address, double, double>> gateways;
};

struct AdrCandidate
{
    int spreadingFactor = 12;
    int transmitPowerDbm = 14;
    int channelIndex = 0;
    double predictedSuccess = 0;
    double linkMarginDb = 0;
    double energyCostJ = 0;
    bool feasible = false;
};

enum class AdrAlgorithm
{
    LORAWAN_REFERENCE = 1,
    THESIS_LEGACY = 2,
    MOBILITY_ENERGY = 3,
    ND_ADR = 4
    , MADERE = 5
};

/**
 * Network server with three explicitly selectable ADR paths.
 *
 * lorawanReference reproduces the SNR-margin algorithm documented in the
 * thesis/papers; thesisLegacy preserves the distance/channel proposal; and
 * mobilityEnergy adds per-node classification, hysteresis and minimization of
 * (Etx + Erx1 + Erx2 + Esleep) / Psuccess.
 */
class NetworkServerApp : public cSimpleModule, public cListener
{
  protected:
    UdpSocket socket;
    int localPort = -1;
    int destPort = -1;
    bool evaluateAdr = false;
    AdrAlgorithm adrAlgorithm = AdrAlgorithm::THESIS_LEGACY;
    std::string adrAlgorithmName;
    bool useCsiNotificationsForAdr = true;
    // Kept only so old INI files that used the Boolean still resolve through
    // adrAlgorithm="compatibility". New experiments use adrAlgorithm directly.
    bool adaptiveMobilityAdr = false;
    std::string adrMethod;
    double adrDeviceMargin = 15;
    double adrStepSize = 3;
    int adrWindowSize = 20;
    double proposalMaxDistance = 15000;
    bool enableM1 = true;
    bool enableM2 = true;
    bool forcePeriodicAdrResponse = true;

    int staticAdrWindowSize = 20;
    int mobileAdrWindowSize = 5;
    double ewmaAlpha = 0.4;
    double mobilitySpeedThreshold = 0.2;
    int mobileConfirmationSamples = 3;
    int staticConfirmationSamples = 8;
    double staticAdrMargin = 10;
    double mobileAdrMargin = 15;
    double sfBoundaryHysteresis = 250;
    int sfConfirmationSamples = 3;
    double targetPdr = 0.9;
    double reliabilitySlope = 2;
    double channelLoadPenalty = 0.25;
    double collisionModelScale = 2.0;
    int channelSwitchLoadHysteresis = 1;
    bool constrainMobileSfByDistance = true;
    double energyEvaluationPeriod = 1000;
    double rxWindowDuration = 1;
    int rxWindowCount = 2;
    double stableHeartbeatInterval = 43200;
    double activeHeartbeatInterval = 3600;
    double initialHeartbeatInterval = 21600;
    double deteriorationSnrDrop = 3;
    double deteriorationRssiDrop = 6;
    int deteriorationConfirmationSamples = 3;
    int deteriorationHoldSamples = 3;
    int maxTpDecreasePerCommand = 1;
    int maxTpIncreasePerCommand = 3;

    // Electrical values are read from the same XML as LoRaEnergyConsumer.
    double supplyVoltage = 0;
    double receiverCurrentMa = 0;
    double sleepCurrentMa = 0;
    std::map<int, double> txCurrentMaByDbm;

    std::vector<KnownNode> knownNodes;
    std::vector<PendingUplink> pendingUplinks;
    const std::vector<double> eu868ChannelsMhz = {
        868.1, 868.3, 868.5, 867.1, 867.3, 867.5, 867.7, 867.9
    };

    long sentDataPackets = 0;
    long sentDataPacketsPerSf[6] = {};
    long uniqueDataPackets = 0;
    long uniqueDataPacketsPerSf[6] = {};
    long totalReceivedCopies = 0;
    long heartbeatCopies = 0;
    long csiNotifications = 0;
    long uniqueDataPacketsPerChannel[8] = {};
    std::map<int, int> receivedPerNode;
    std::map<int, int> sentPerNode;
    std::map<int, double> lastSnrPerNode;
    std::map<int, double> lastRssiPerNode;
    cHistogram receivedRssi;

    simsignal_t packetReceivedSignal;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;
    virtual void finish() override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalId,
            intval_t value, cObject *details) override;

    void startUdp();
    void handleUplink(Packet *packet);
    void addToProcessingTable(Packet *packet);
    void processPendingUplink(cMessage *timer);
    void clearPendingUplinks();

    KnownNode& updateKnownNode(const Ptr<const LoRaMacFrame>& frame);
    bool isOldPacket(const Ptr<const LoRaMacFrame>& frame) const;
    int findPendingUplink(const Ptr<const LoRaMacFrame>& frame) const;
    int findNode(const MacAddress& address) const;
    int findLeastLoadedChannel() const;
    std::vector<int> channelLoads() const;

    void evaluateAdrCommand(Packet *packet, KnownNode& node,
            const L3Address& gateway, double bestSnr, double bestRssi);
    void evaluateLoRaWanReferenceAdrCommand(Packet *packet, KnownNode& node,
            const L3Address& gateway, double bestSnr);
    void evaluateThesisLegacyAdrCommand(Packet *packet, KnownNode& node,
            const L3Address& gateway, double bestSnr);
    void evaluateAdaptiveAdrCommand(Packet *packet, KnownNode& node,
            const L3Address& gateway, double bestSnr, double bestRssi);
    void evaluateNdAdrCommand(Packet *packet, KnownNode& node,
            const L3Address& gateway, double bestSnr, double bestRssi);
    void evaluateMadereCommand(Packet *packet, KnownNode& node,
            const L3Address& gateway, double bestSnr);
    void updateAdaptiveObservation(KnownNode& node,
            const Ptr<const LoRaMacFrame>& frame,
            const Ptr<const LoRaAppPacket>& appPacket, double bestSnr, double bestRssi);
    AdrCandidate selectEnergyCandidate(const KnownNode& node,
            const Ptr<const LoRaMacFrame>& frame, int payloadBytes,
            int fixedSpreadingFactor, double safetyMargin) const;
    double estimateEnergyCost(const Ptr<const LoRaMacFrame>& frame,
            int payloadBytes, int sf, int tpDbm, double successProbability) const;
    double predictSuccess(const KnownNode& node, const Ptr<const LoRaMacFrame>& frame,
            int payloadBytes, int sf, int tpDbm, int channelIndex,
            const std::vector<int>& loads) const;
    int confirmedMobileSpreadingFactor(KnownNode& node, double distance);
    void sendAdrCommand(const Ptr<const LoRaMacFrame>& frame, KnownNode& node,
            const L3Address& gateway, int sf, int tpDbm, int channelIndex,
            double heartbeatInterval, int mobilityClass, bool forceResponse,
            double feedbackSnr = -999);
    void loadEnergyConfiguration();
    double getTxCurrentMa(int txPowerDbm) const;

    void sendConfirmedAck(const Ptr<const LoRaMacFrame>& frame,
            const Ptr<const LoRaAppPacket>& appPacket,
            const L3Address& gateway);

    static double requiredSnr(int spreadingFactor);
    static bool isAccountingPacket(const Ptr<const LoRaMacFrame>& frame,
            const Ptr<const LoRaAppPacket>& appPacket);

  public:
    NetworkServerApp() = default;
    virtual ~NetworkServerApp() override;
};

} // namespace flora

#endif
