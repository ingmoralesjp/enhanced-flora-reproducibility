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

#include "LoRaPhy/LoRaAnalogModel.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarAnalogModel.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarReception.h"
#include "LoRaReception.h"
#include "LoRaTransmission.h"
#include "LoRaReceiver.h"
#include "LoRa/LoRaRadio.h"

namespace flora {

Define_Module(LoRaAnalogModel);

std::ostream& LoRaAnalogModel::printToStream(std::ostream& stream, int level, int evFlags) const
{
    return stream << "LoRaAnalogModel";
}

const W LoRaAnalogModel::getBackgroundNoisePower(const LoRaBandListening *listening) const
{
    const std::string noiseModel = par("backgroundNoiseModel").stdstringValue();
    if (noiseModel == "legacySensitivity") {
        // Exact table recovered from the inherited FLoRa analog model. These
        // are receiver-sensitivity values used as background noise.
        static const double legacyNoiseDbm[6][3] = {
            {-124, -122, -116}, // SF7
            {-127, -125, -119}, // SF8
            {-130, -128, -122}, // SF9
            {-133, -130, -125}, // SF10
            {-135, -132, -128}, // SF11
            {-137, -135, -129}  // SF12
        };
        const int sf = listening->getLoRaSF();
        const double bandwidthHz = listening->getLoRaBW().get();
        int bandwidthIndex = bandwidthHz == 250000.0 ? 1 :
                bandwidthHz == 500000.0 ? 2 : 0;
        const double noisePowerDbm = sf >= 7 && sf <= 12 ?
                legacyNoiseDbm[sf - 7][bandwidthIndex] : -126.5;
        return W(math::dBmW2mW(noisePowerDbm) / 1000);
    }
    if (noiseModel != "thermal")
        throw cRuntimeError("backgroundNoiseModel must be thermal or legacySensitivity");

    // The medium performs an initialization-time probe with an unspecified
    // band; use the thesis' 125 kHz default for that probe.
    const double configuredBandwidthHz = listening->getLoRaBW().get();
    const double bandwidthHz = configuredBandwidthHz > 0 ? configuredBandwidthHz :
            par("fallbackNoiseBandwidth").doubleValue();

    // Sensitivity is checked separately by LoRaReceiver. The noise model
    // contains thermal noise plus the receiver noise figure only.
    const double noisePowerDbm = par("thermalNoiseDensityDbmHz").doubleValue()
            + 10 * log10(bandwidthHz) + par("receiverNoiseFigureDb").doubleValue();
    return W(math::dBmW2mW(noisePowerDbm) / 1000);
}

void LoRaAnalogModel::finish()
{
    const std::string noiseModel = par("backgroundNoiseModel").stdstringValue();
    recordScalar("backgroundNoiseModelCode", noiseModel == "thermal" ? 1 : 0);
    if (noiseModel == "thermal") {
        recordScalar("thermalNoiseDensityDbmHz", par("thermalNoiseDensityDbmHz").doubleValue());
        recordScalar("receiverNoiseFigureDb", par("receiverNoiseFigureDb").doubleValue(), "dB");
    }
}

W LoRaAnalogModel::computeReceptionPower(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
{
    const IRadioMedium *radioMedium = receiverRadio->getMedium();
//    const IRadio *transmitterRadio = transmission->getTransmitter();
//    const IAntenna *receiverAntenna = receiverRadio->getAntenna();
//    const IAntenna *transmitterAntenna = transmitterRadio->getAntenna();
    const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(transmission->getAnalogModel());
    const IScalarSignal *scalarSignalAnalogModel = check_and_cast<const IScalarSignal *>(transmission->getAnalogModel());
    const Coord receptionStartPosition = arrival->getStartPosition();
    const Coord receptionEndPosition = arrival->getEndPosition();
//    const Quaternion transmissionDirection = computeTransmissionDirection(transmission, arrival);
//    const Quaternion transmissionAntennaDirection = transmission->getStartOrientation() - transmissionDirection;
//    const Quaternion receptionAntennaDirection = transmissionDirection - arrival->getStartOrientation();
    double transmitterAntennaGain = computeAntennaGain(transmission->getTransmitterAntennaGain(), transmission->getStartPosition(), arrival->getStartPosition(), transmission->getStartOrientation());
    double receiverAntennaGain = computeAntennaGain(receiverRadio->getAntenna()->getGain().get(), arrival->getStartPosition(), transmission->getStartPosition(), arrival->getStartOrientation());
    double pathLoss = radioMedium->getPathLoss()->computePathLoss(transmission, arrival);
    double obstacleLoss = radioMedium->getObstacleLoss() ? radioMedium->getObstacleLoss()->computeObstacleLoss(narrowbandSignalAnalogModel->getCenterFrequency(), transmission->getStartPosition(), receptionStartPosition) : 1;
    W transmissionPower = scalarSignalAnalogModel->getPower();
    return transmissionPower * std::min(1.0, transmitterAntennaGain * receiverAntennaGain * pathLoss * obstacleLoss);
}

const IReception *LoRaAnalogModel::computeReception(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
{
    const LoRaTransmission *loRaTransmission = check_and_cast<const LoRaTransmission *>(transmission);
    const simtime_t receptionStartTime = arrival->getStartTime();
    const simtime_t receptionEndTime = arrival->getEndTime();
    const Quaternion receptionStartOrientation = arrival->getStartOrientation();
    const Quaternion receptionEndOrientation = arrival->getEndOrientation();
    const Coord receptionStartPosition = arrival->getStartPosition();
    const Coord receptionEndPosition = arrival->getEndPosition();
    W receivedPower = computeReceptionPower(receiverRadio, transmission, arrival);
    Hz LoRaCF = loRaTransmission->getLoRaCF();
    int LoRaSF = loRaTransmission->getLoRaSF();
    Hz LoRaBW = loRaTransmission->getLoRaBW();
    int LoRaCR = loRaTransmission->getLoRaCR();
    return new LoRaReception(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition, receptionStartOrientation, receptionEndOrientation, LoRaCF, LoRaBW, receivedPower, LoRaSF, LoRaCR);
}

const INoise *LoRaAnalogModel::computeNoise(const IListening *listening, const IInterference *interference) const
{
    const LoRaBandListening *bandListening = check_and_cast<const LoRaBandListening *>(listening);
    Hz commonCarrierFrequency = bandListening->getLoRaCF();
    Hz commonBandwidth = bandListening->getLoRaBW();
    simtime_t noiseStartTime = SimTime::getMaxTime();
    simtime_t noiseEndTime = 0;
    std::map<simtime_t, W> *powerChanges = new std::map<simtime_t, W>();
    const std::vector<const IReception *> *interferingReceptions = interference->getInterferingReceptions();
    for (auto reception : *interferingReceptions) {
        const ISignalAnalogModel *signalAnalogModel = reception->getAnalogModel();
        const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(signalAnalogModel);
        const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(signalAnalogModel);
        Hz signalCarrierFrequency = loRaReception->getLoRaCF();
        Hz signalBandwidth = loRaReception->getLoRaBW();
        if((commonCarrierFrequency == signalCarrierFrequency && commonBandwidth == signalBandwidth))
        {
            const IScalarSignal *scalarSignalAnalogModel = check_and_cast<const IScalarSignal *>(signalAnalogModel);
            W power = scalarSignalAnalogModel->getPower();
            simtime_t startTime = reception->getStartTime();
            simtime_t endTime = reception->getEndTime();
            if (startTime < noiseStartTime)
                noiseStartTime = startTime;
            if (endTime > noiseEndTime)
                noiseEndTime = endTime;
            std::map<simtime_t, W>::iterator itStartTime = powerChanges->find(startTime);
            if (itStartTime != powerChanges->end())
                itStartTime->second += power;
            else
                powerChanges->insert(std::pair<simtime_t, W>(startTime, power));
            std::map<simtime_t, W>::iterator itEndTime = powerChanges->find(endTime);
            if (itEndTime != powerChanges->end())
                itEndTime->second -= power;
            else
                powerChanges->insert(std::pair<simtime_t, W>(endTime, -power));
        }
        else if (areOverlappingBands(commonCarrierFrequency, commonBandwidth, narrowbandSignalAnalogModel->getCenterFrequency(), narrowbandSignalAnalogModel->getBandwidth()))
            throw cRuntimeError("Overlapping bands are not supported");
    }

    simtime_t startTime = listening->getStartTime();
    simtime_t endTime = listening->getEndTime();
    std::map<simtime_t, W> *backgroundNoisePowerChanges = new std::map<simtime_t, W>();
    const W noisePower = getBackgroundNoisePower(bandListening);
    backgroundNoisePowerChanges->insert(std::pair<simtime_t, W>(startTime, noisePower));
    backgroundNoisePowerChanges->insert(std::pair<simtime_t, W>(endTime, -noisePower));

    for (const auto & backgroundNoisePowerChange : *backgroundNoisePowerChanges) {
        std::map<simtime_t, W>::iterator jt = powerChanges->find(backgroundNoisePowerChange.first);
        if (jt != powerChanges->end())
            jt->second += backgroundNoisePowerChange.second;
        else
            powerChanges->insert(std::pair<simtime_t, W>(backgroundNoisePowerChange.first, backgroundNoisePowerChange.second));
    }

    EV_TRACE << "Noise power begin " << endl;
    W noise = W(0);
    for (std::map<simtime_t, W>::const_iterator it = powerChanges->begin(); it != powerChanges->end(); it++) {
        noise += it->second;
        EV_TRACE << "Noise at " << it->first << " = " << noise << endl;
    }
    EV_TRACE << "Noise power end" << endl;
    return new ScalarNoise(noiseStartTime, noiseEndTime, commonCarrierFrequency, commonBandwidth, powerChanges);
}

const ISnir *LoRaAnalogModel::computeSNIR(const IReception *reception, const INoise *noise) const
{
    return new ScalarSnir(reception, noise);
}

} // namespace inet
