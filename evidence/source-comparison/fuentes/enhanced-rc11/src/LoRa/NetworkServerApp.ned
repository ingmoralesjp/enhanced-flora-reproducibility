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

package flora.LoRa;
import inet.applications.contract.IApp;

simple NetworkServerApp like IApp
{
    @signal[LoRa_ServerPacketReceived](type=bool); // optional
    @statistic[LoRa_ServerPacketReceived](source=LoRa_ServerPacketReceived; record=count);
    int localPort = default(-1);  // local port (-1: use ephemeral port)
    string localAddress = default("");
    int destPort = default(-1);
    bool evaluateADRinServer = default(false);
    int headerLength @unit(B) = default(8B);

    string adrMethod = default("max");
    double adrDeviceMargin = default(15);//default 15 TTN
    // The new ablation campaign uses 2.5 dB to reproduce The Things Stack.
    // Existing campaigns retain their historical 3 dB default unless overridden.
    double adrStepSize @unit(dB) = default(3dB);
    int adrWindowSize = default(20);
    double proposalMaxDistance @unit(m) = default(15000m);

    // Orthogonal switches for the paper's 2x2 factorial ablation. Defaults
    // preserve the historical thesisLegacy behavior in existing INI files.
    bool enableM1 = default(true);  // balanced selection over eight EU868 channels
    bool enableM2 = default(true);  // distance-guided spreading factor
    bool forcePeriodicAdrResponse = default(true); // historical compatibility

    // Explicit research treatments. "compatibility" maps the deprecated
    // adaptiveMobilityAdr Boolean used by pre-2026 INI files.
    string adrAlgorithm = default("compatibility"); // lorawanReference, thesisLegacy, mobilityEnergy, ndAdr, madere
    // Gateway-generated CSI frames represent failed receptions. They are an
    // enhanced-FLoRa capability and must be explicitly enabled per treatment.
    bool useCsiNotificationsForAdr = default(true);

    // Deprecated compatibility switch; do not use in new experiment configs.
    bool adaptiveMobilityAdr = default(false);
    int staticAdrWindowSize = default(20);  // stable nodes: slower decisions
    int mobileAdrWindowSize = default(5);   // mobile/unknown nodes: quick reaction
    double ewmaAlpha = default(0.4);
    double mobilitySpeedThreshold @unit(mps) = default(0.2mps);
    int mobileConfirmationSamples = default(3);
    int staticConfirmationSamples = default(8);
    double staticAdrMargin @unit(dB) = default(10dB);
    double mobileAdrMargin @unit(dB) = default(15dB);
    double sfBoundaryHysteresis @unit(m) = default(250m);
    int sfConfirmationSamples = default(3);
    double targetPdr = default(0.9);
    double reliabilitySlope @unit(dB) = default(2dB);
    double channelLoadPenalty = default(0.25);
    // Pure ALOHA collision exponent: exp(-scale * contenders * ToA / period).
    double collisionModelScale = default(2.0);
    int channelSwitchLoadHysteresis = default(1);
    // Compatibility mode preserves the former distance-band SF constraint.
    // V2 can let reliability/energy select SF for mobile nodes as well.
    bool constrainMobileSfByDistance = default(true);

    // Cost = (Etx + Erx1 + Erx2 + Esleep) / Psuccess. The XML is shared with
    // LoRaEnergyConsumer, so optimization and measured energy use one model.
    xml energyConfigFile = default(xml("<energy/>"));
    double energyEvaluationPeriod @unit(s) = default(1000s);
    double rxWindowDuration @unit(s) = default(1s);
    int rxWindowCount = default(2);

    // Heartbeats stay sparse for a stable static node and become frequent only
    // while motion is present or recent link deterioration is being tracked.
    double stableHeartbeatInterval @unit(s) = default(12h);
    double activeHeartbeatInterval @unit(s) = default(1h);
    double initialHeartbeatInterval @unit(s) = default(6h);
    double deteriorationSnrDrop @unit(dB) = default(3dB);
    double deteriorationRssiDrop @unit(dB) = default(6dB);
    int deteriorationConfirmationSamples = default(3); // reject isolated shadow-fading excursions
    int deteriorationHoldSamples = default(3);
    int maxTpDecreasePerCommand = default(1);
    int maxTpIncreasePerCommand = default(3);

    gates:
    output socketOut @labels(UdpControlInfo/up);
    input socketIn @labels(UdpControlInfo/down);

}
