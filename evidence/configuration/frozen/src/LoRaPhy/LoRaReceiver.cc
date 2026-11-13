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

#include "LoRaReceiver.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarNoise.h"
#include "../LoRaApp/SimpleLoRaApp.h"
#include "LoRaPhyPreamble_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

namespace flora {

Define_Module(LoRaReceiver);

LoRaReceiver::LoRaReceiver() :
    snirThreshold(NaN)
{
}

void LoRaReceiver::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        snirThreshold = math::dB2fraction(par("snirThreshold"));
        if(strcmp(getParentModule()->getClassName(), "flora::LoRaGWRadio") == 0)
        {
            iAmGateway = true;
        } else iAmGateway = false;
        alohaChannelModel = par("alohaChannelModel");
        enforceRequiredSnr = par("enforceRequiredSnr");
        LoRaReceptionCollision = registerSignal("LoRaReceptionCollision");
        numCollisions = 0;
        numLowSNRLosses = 0;
        snrsVector.setName("LoraReceiver SNRs Vector");
        miRSSIVector.setName("LoraReceiver RSSIs Vector");

    }
}

void LoRaReceiver::finish()
{
        recordScalar("numCollisions", numCollisions);
        recordScalar("rcvBelowSensitivity_NotCollided", rcvBelowSensitivity_NotCollided);
        recordScalar("numLowSNRLosses", numLowSNRLosses);  // New counter for SNR-based losses!
        recordScalar("numCollisionsBelowSens", numCollisionsBelowSens);  // Log new counter
        recordScalar("numCollisionsAboveSens", numCollisionsAboveSens);
        recordScalar("Total_rcvBelowSensitivity", Total_rcvBelowSensitivity);
        recordScalar("otherPhyLosses", otherPhyLosses);
        recordScalar("requiredSnrGateEnabled", enforceRequiredSnr);


        ////---------------////////////////////////////////////////-----------------------------------------------------------
       snr_media=miSNRCountTotal.getMean();
       snr_max=miSNRCountTotal.getMax();
       snr_dev=miSNRCountTotal.getStddev();
       snr_min=miSNRCountTotal.getMin();
       recordScalar("snr_media", snr_media);
       recordScalar("snr_max", snr_max);
       recordScalar("snr_min", snr_min);
       recordScalar("snr_dev", snr_dev);

       rssi_media=miRSSICountTotal.getMean();
       rssi_max=miRSSICountTotal.getMax();
       rssi_dev=miRSSICountTotal.getStddev();
       rssi_min=miRSSICountTotal.getMin();
       recordScalar("rssi_media", rssi_media);
       recordScalar("rssi_max", rssi_max);
       recordScalar("rssi_min", rssi_min);
       recordScalar("rssi_dev", rssi_dev);
       flaggedBelowSens.clear(); // Optional cleanup
       collidedPackets.clear(); // Optional cleanup

}

bool LoRaReceiver::computeIsReceptionPossible(const IListening *listening, const ITransmission *transmission) const
{
    //here we can check compatibility of LoRaTx parameters (or beeing a gateway)
    const LoRaTransmission *loRaTransmission = check_and_cast<const LoRaTransmission *>(transmission);
    auto *loRaApp = check_and_cast<SimpleLoRaApp *>(getParentModule()->getParentModule()->getSubmodule("SimpleLoRaApp"));
    if (iAmGateway || (loRaTransmission->getLoRaCF() == loRaApp->getLoRaCF()
            && loRaTransmission->getLoRaBW() == loRaApp->getLoRaBW()
            && loRaTransmission->getLoRaSF() == loRaApp->getLoRaSF()))
        return true;
    else
        return false;
}

bool LoRaReceiver::computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const
{
    //here we can check compatibility of LoRaTx parameters (or beeing a gateway) and reception above sensitivity level
    const LoRaBandListening *loRaListening = check_and_cast<const LoRaBandListening *>(listening);
    const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(reception);
    if (iAmGateway == false && (loRaListening->getLoRaCF() != loRaReception->getLoRaCF() || loRaListening->getLoRaBW() != loRaReception->getLoRaBW() || loRaListening->getLoRaSF() != loRaReception->getLoRaSF())) {
        return false;
    }
    else
    {
        W minReceptionPower = loRaReception->computeMinPower(reception->getStartTime(part), reception->getEndTime(part));
        W sensitivity = getSensitivity(loRaReception);

        bool isReceptionPossible = minReceptionPower >= sensitivity;

        EV_DEBUG << "Computing whether reception is possible: minimum reception power = " << minReceptionPower << ", sensitivity = " << sensitivity << " -> reception is " << (isReceptionPossible ? "possible" : "impossible") << endl;

        // Check and register packets that were below sensitivity only once
           if(isReceptionPossible == false)
        {
            if (simTime() >= getSimulation()->getWarmupPeriod()) //mi linea solo contar despues de warmup
            {
                const Packet* pkt = reception->getTransmission()->getPacket();
                // Packet already flagged as below sensitivity, skip double count
                if (flaggedBelowSens.find(pkt) == flaggedBelowSens.end())// Avoid double-counting
                {
                    Total_rcvBelowSensitivity++;
                    //const_cast<LoRaReceiver*>(this)->Total_rcvBelowSensitivity++;//no lo necesito con mutable
                    // colisiono???

                    if (collidedPackets.find(pkt) == collidedPackets.end())
                        rcvBelowSensitivity_NotCollided++;
                        //const_cast<LoRaReceiver* >(this)->rcvBelowSensitivity_NotCollided++;//no lo necesito con mutable
                    flaggedBelowSens.insert(pkt);

                }
            }
        }
        return isReceptionPossible;
    }
}

bool LoRaReceiver::computeIsReceptionAttempted(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const
{
    if(isPacketCollided(reception, part, interference))
    {
        // Marcar como colisionado // Register collided packets only once to prevent double-counting
        const Packet* pkt = reception->getTransmission()->getPacket();
        collidedPackets.insert(pkt);
        //const_cast<LoRaReceiver*>(this)->collidedPackets.insert(pkt);//no lo necesito con mutable
        // Marcar como colisionado

        auto packet = reception->getTransmission()->getPacket();
        const auto &chunk = packet->peekAtFront<FieldsChunk>();
        auto loraMac = dynamicPtrCast<const LoRaMacFrame>(chunk);

        auto loraPreamble = dynamicPtrCast<const LoRaPhyPreamble>(chunk);
        MacAddress rec;
            if (loraPreamble)
                rec = loraPreamble->getReceiverAddress();
            else if (loraMac)
                rec = loraMac->getReceiverAddress();

            if (iAmGateway == false) //If the receiver is an end device, it checks if the packet was meant for this node.
                {
                    auto *macLayer = check_and_cast<LoRaMac *>(getParentModule()->getParentModule()->getSubmodule("mac"));
                    if (rec == macLayer->getAddress())
                        {
                     //       const_cast<LoRaReceiver* >(this)->numCollisions++;
                        }
                    //EV << "Node: Extracted macFrame = " << loraMacFrame->getReceiverAddress() << ", node address = " << macLayer->getAddress() << std::endl;
                }
            else //If the receiver is a gateway, it compares the extracted address with its own.
                {
                    auto *gwMacLayer = check_and_cast<LoRaGWMac *>(getParentModule()->getParentModule()->getSubmodule("mac"));
                    EV << "GW: Extracted macFrame = " << rec << ", node address = " << gwMacLayer->getAddress() << std::endl;
                    if (rec == MacAddress::BROADCAST_ADDRESS)//BROADCAST_ADDRESS es la direccion del GW
                        {
                              if (simTime() >= getSimulation()->getWarmupPeriod())//mi linea solo contar despues de warmup
                              {

                                         numCollisions++;
                                         //const_cast<LoRaReceiver* >(this)->numCollisions++;

                                         // Count collisions that were also below sensitivity
                                         const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(reception);
                                         W minReceptionPower = loRaReception->computeMinPower(reception->getStartTime(part), reception->getEndTime(part));
                                         W sensitivity = getSensitivity(loRaReception);
                                         if (minReceptionPower < sensitivity)
                                         {
                                             numCollisionsBelowSens++;
                                             //const_cast<LoRaReceiver*>(this)->numCollisionsBelowSens++;//no lo necesito con mutable
                                         }

                                         else
                                             numCollisionsAboveSens++;
                                             //const_cast<LoRaReceiver*>(this)->numCollisionsAboveSens++;//no lo necesito con mutable

                              }
                        }
                }
            return false; //hay colision, no se considera intento de recepcion
    }
    else
    {
        return true; //no hay colision se considera intento de recepcion
    }
}

bool LoRaReceiver::isPacketCollided(const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const
{
    //Takes a received packet (reception), a part of the signal (part), and a list of ongoing transmissions (interference).
    //Retrieves all signals that were transmitted at the same time as the reception packet.
    auto interferingReceptions = interference->getInterferingReceptions();
    const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(reception); ///esta captura la señal *reception
    simtime_t m_x = (loRaReception->getStartTime() + loRaReception->getEndTime())/2;
    simtime_t d_x = (loRaReception->getEndTime() - loRaReception->getStartTime())/2;
    //calculates midpoint (m_y) and duration (d_y) for the reception packet.

    EV << "El tiempo de transmisión es " << loRaReception->getEndTime() - loRaReception->getStartTime() << endl;
    W signalRSSI_w = loRaReception->getPower();  ///// estas CAPTURA la potencia
    double signalRSSI_mw = signalRSSI_w.get()*1000; ///// estas convierte la potencia en mW
    double signalRSSI_dBm = math::mW2dBmW(signalRSSI_mw); ///// estas convierte la potencia en dBm

    EV << signalRSSI_mw << endl;
    EV << signalRSSI_dBm << endl;
    int receptionSF = loRaReception->getLoRaSF();

    for (auto interferingReception : *interferingReceptions)

    {
        bool overlap = false;
        bool frequencyCollision = false;
        bool captureEffect = false;
        bool timingCollision = false; //Collision is acceptable in first part of preamble
        const LoRaReception *loRaInterference = check_and_cast<const LoRaReception *>(interferingReception);///esta captura la señal *interference

        simtime_t m_y = (loRaInterference->getStartTime() + loRaInterference->getEndTime())/2;
        simtime_t d_y = (loRaInterference->getEndTime() - loRaInterference->getStartTime())/2;
        //calculates midpoint (m_y) and duration (d_y) for the interfering packet.
        if(omnetpp::fabs(m_x - m_y) < d_x + d_y)
        {
            overlap = true; //se marca el indicador que coincidideron en el tiempo las dos senales
        }

        if(loRaReception->getLoRaCF() == loRaInterference->getLoRaCF())
        {
            frequencyCollision = true; //se marca que coinciden en CF


        }

        W interferenceRSSI_w = loRaInterference->getPower();
        double interferenceRSSI_mw = interferenceRSSI_w.get()*1000;
        double interferenceRSSI_dBm = math::mW2dBmW(interferenceRSSI_mw);
        int interferenceSF = loRaInterference->getLoRaSF();

        /* If difference in power between two signals is greater than threshold, no collision*/
        if(signalRSSI_dBm - interferenceRSSI_dBm >= nonOrthDelta[receptionSF-7][interferenceSF-7])
        {
            // Compares the received power with the interference power using a lookup table (nonOrthDelta).
            //If the received packet is significantly stronger than the interference, the receiver ignores the interference
            captureEffect = true;
        }

        EV << "[MSDEBUG] Received packet at SF: " << receptionSF << " with power " << signalRSSI_dBm << endl;
        EV << "[MSDEBUG] Received interference at SF: " << interferenceSF << " with power " << interferenceRSSI_dBm << endl;
        EV << "[MSDEBUG] Acceptable diff is equal " << nonOrthDelta[receptionSF-7][interferenceSF-7] << endl;
        EV << "[MSDEBUG] Diff is equal " << signalRSSI_dBm - interferenceRSSI_dBm << endl;

        if (captureEffect == false)
        {
            EV << "[MSDEBUG] Packet is discarded" << endl;
        }

        else
            EV << "[MSDEBUG] Packet is not discarded" << endl;

        /* If last 6 symbols of preamble are received, no collision*/
        //If the interference overlaps with the last 6 symbols of the preamble, it causes a timing collision.
        double nPreamble = 8; //from the paper "Do Lora networks..."
        simtime_t Tsym = (pow(2, loRaReception->getLoRaSF()))/(loRaReception->getLoRaBW().get()/1000)/1000;
        simtime_t csBegin = loRaReception->getPreambleStartTime() + Tsym * (nPreamble - 6);
        if(csBegin < loRaInterference->getEndTime())
        {
            timingCollision = true;
        }

        if (overlap && frequencyCollision)
        {
            if(alohaChannelModel == true)
            {
                // If ALOHA-based transmission is used, any overlap + frequency collision = a packet collision.
                if(iAmGateway && (part == IRadioSignal::SIGNAL_PART_DATA || part == IRadioSignal::SIGNAL_PART_WHOLE)) const_cast<LoRaReceiver* >(this)->emit(LoRaReceptionCollision, true);
                return true;
            }
            if(alohaChannelModel == false)
            {
                if(captureEffect == false && timingCollision)
                {
                    //If ALOHA is not used, a collision occurs only if the capture effect fails AND there's a timing collision.
                    if(iAmGateway && (part == IRadioSignal::SIGNAL_PART_DATA || part == IRadioSignal::SIGNAL_PART_WHOLE)) const_cast<LoRaReceiver* >(this)->emit(LoRaReceptionCollision, true);
                    return true;
                }//fin if captureEffect

            }//fin If ALOHA is not used


        }//fin if overlap

        if  ((overlap && frequencyCollision) || (captureEffect == false && timingCollision))/// este if lo puse yo
        {
            //potencia recibida//--------------------------------------//////////////////////////////-------INICIO LINEAS MIAS
                           W signalRSSI_w = loRaReception->getPower();  ///// estas CAPTURA la potencia
                              double signalRSSI_mw = signalRSSI_w.get()*1000; ///// estas convierte la potencia en mW
                              double signalRSSI_dBm = math::mW2dBmW(signalRSSI_mw); ///// estas convierte la potencia en dBm
                           //noise----///
                              miRSSI_GW = signalRSSI_dBm;
                               // Constants for noise floor calculation
                                   if(loRaReception->getLoRaBW() == Hz(125000)) miBW=125000;
                                   if(loRaReception->getLoRaBW() == Hz(250000)) miBW=250000;
                                   if(loRaReception->getLoRaBW() == Hz(500000)) miBW=250000;
                                   if(loRaReception->getLoRaSF() == 7) miSNRreq = -7.5;
                                   if(loRaReception->getLoRaSF() == 8) miSNRreq = -10;
                                   if(loRaReception->getLoRaSF() == 9) miSNRreq = -12.5;
                                   if(loRaReception->getLoRaSF() == 10) miSNRreq = -15;
                                   if(loRaReception->getLoRaSF() == 11) miSNRreq = -17.5;
                                   if(loRaReception->getLoRaSF() == 12) miSNRreq = -20;
                                   double NF = 6;  // Noise Figure in dB
                                   //double B = miBW;  // Channel bandwidth in Hz
                                   double k = -174;  // Theoretical noise floor at room temperature in dBm/Hz for 1 Hz bandwidth
                                   double B_adjusted = miBW * 1.5;  // Adjusting the bandwidth for GFSK modulation

                                   // Calculate the TXVR noise floor incorporating the receiver's noise figure and adjusted bandwidth
                                   noiseFloor = k + 10 * log10(B_adjusted) + NF+ miSNRreq;  // Calculated noise floor based on SF
                                   miSNR_GW = miRSSI_GW - noiseFloor;
                                   miRSSIVector.record(miRSSI_GW);
                                   miRSSICountTotal.collect(miRSSI_GW);
                                   snrsVector.record(miSNR_GW);
                                   miSNRCountTotal.collect(miSNR_GW);

                                  ///noise//-----------------------------------

        }


    }//fin for

    return false;
}

const IReceptionDecision *LoRaReceiver::computeReceptionDecision(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const
{
    auto isReceptionPossible = computeIsReceptionPossible(listening, reception, part);
    auto isReceptionAttempted = isReceptionPossible && computeIsReceptionAttempted(listening, reception, part, interference);
    auto isReceptionSuccessful = isReceptionAttempted && computeIsReceptionSuccessful(listening, reception, part, interference, snir);
    return new ReceptionDecision(reception, part, isReceptionPossible, isReceptionAttempted, isReceptionSuccessful);

}

Packet *LoRaReceiver::computeReceivedPacket(const ISnir *snir, bool isReceptionSuccessful) const
{
    auto transmittedPacket = snir->getReception()->getTransmission()->getPacket();
    auto receivedPacket = transmittedPacket->dup();
    receivedPacket->clearTags();
//    receivedPacket->addTag<PacketProtocolTag>()->setProtocol(transmittedPacket->getTag<PacketProtocolTag>()->getProtocol());
    if (!isReceptionSuccessful)
        receivedPacket->setBitError(true);
    return receivedPacket;
}

const IReceptionResult *LoRaReceiver::computeReceptionResult(const IListening *listening, const IReception *reception, const IInterference *interference, const ISnir *snir, const std::vector<const IReceptionDecision *> *decisions) const
{
    bool isReceptionSuccessful = true;
    for (auto decision : *decisions)
        isReceptionSuccessful &= decision->isReceptionSuccessful();
    auto packet = computeReceivedPacket(snir, isReceptionSuccessful);

    auto signalPowerInd = packet->addTagIfAbsent<SignalPowerInd>();
    const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(reception);
    W signalRSSI_w = loRaReception->getPower();
    signalPowerInd->setPower(signalRSSI_w);
////////////////////////////////////////////////////////////////////////////////////////////necesito saber mas de esto////////////////////////////////////////
    auto snirInd = packet->addTagIfAbsent<SnirInd>();
    snirInd->setMinimumSnir(snir->getMin());
    snirInd->setMaximumSnir(snir->getMax());
    auto signalTimeInd = packet->addTagIfAbsent<SignalTimeInd>();
    signalTimeInd->setStartTime(reception->getStartTime());
    signalTimeInd->setEndTime(reception->getEndTime());
    auto errorRateInd = packet->addTagIfAbsent<ErrorRateInd>();
    errorRateInd->setPacketErrorRate(errorModel ? errorModel->computePacketErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
    errorRateInd->setBitErrorRate(errorModel ? errorModel->computeBitErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
    errorRateInd->setSymbolErrorRate(errorModel ? errorModel->computeSymbolErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);

    return new ReceptionResult(reception, decisions, packet);
}

bool LoRaReceiver::computeIsReceptionSuccessful(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const
{
    if (!enforceRequiredSnr)
        return true;

    static const double requiredSnrDb[] = {-7.5, -10, -12.5, -15, -17.5, -20};
    const auto loraReception = check_and_cast<const LoRaReception *>(reception);
    const int spreadingFactor = loraReception->getLoRaSF();
    if (spreadingFactor < 7 || spreadingFactor > 12)
        throw cRuntimeError("Invalid LoRa spreading factor %d", spreadingFactor);

    // Sensitivity determines whether reception can be attempted. Decoding an
    // attempted frame still requires the SF-specific minimum SNIR.
    const double minimumSnrDb = math::fraction2dB(snir->getMin());
    const bool successful = minimumSnrDb >= requiredSnrDb[spreadingFactor - 7];
    if (!successful && simTime() >= getSimulation()->getWarmupPeriod())
        const_cast<LoRaReceiver *>(this)->numLowSNRLosses++;
    return successful;
}

const IListening *LoRaReceiver::createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord &startPosition, const Coord &endPosition) const
{
    if(iAmGateway == false)
    {
        auto node = getContainingNode(this);
        auto loRaApp = check_and_cast<SimpleLoRaApp *>(node->getSubmodule("SimpleLoRaApp"));
        return new LoRaBandListening(radio, startTime, endTime, startPosition, endPosition,
                loRaApp->getLoRaCF(), loRaApp->getLoRaBW(), loRaApp->getLoRaSF());
    }
    else return new LoRaBandListening(radio, startTime, endTime, startPosition, endPosition, LoRaCF, LoRaBW, LoRaSF);
}

const IListeningDecision *LoRaReceiver::computeListeningDecision(const IListening *listening, const IInterference *interference) const
{
    const IRadio *receiver = listening->getReceiver();
    const IRadioMedium *radioMedium = receiver->getMedium();
    const IAnalogModel *analogModel = radioMedium->getAnalogModel();
    const INoise *noise = analogModel->computeNoise(listening, interference);
    const ScalarNoise *loRaNoise = check_and_cast<const ScalarNoise *>(noise);
    W maxPower = loRaNoise->computeMaxPower(listening->getStartTime(), listening->getEndTime());
    bool isListeningPossible = maxPower >= energyDetection;
    delete noise;
    EV_DEBUG << "Computing whether listening is possible: maximum power = " << maxPower << ", energy detection = " << energyDetection << " -> listening is " << (isListeningPossible ? "possible" : "impossible") << endl;
    return new ListeningDecision(listening, isListeningPossible);
}

W LoRaReceiver::getSensitivity(const LoRaReception *reception) const
{
    //function returns sensitivity -- according to LoRa documentation, it changes with LoRa parameters
    //Sensitivity values from Semtech SX1272/73 datasheet, table 10, Rev 3.1, March 2017
    W sensitivity = W(math::dBmW2mW(-126.5) / 1000);
    if(reception->getLoRaSF() == 6)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-121) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-118) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-111) / 1000);
    }

    if (reception->getLoRaSF() == 7)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-124) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-122) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-116) / 1000);
    }

    if(reception->getLoRaSF() == 8)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-127) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-125) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-119) / 1000);
    }
    if(reception->getLoRaSF() == 9)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-130) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-128) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-122) / 1000);
    }
    if(reception->getLoRaSF() == 10)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-133) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-130) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-125) / 1000);
    }
    if(reception->getLoRaSF() == 11)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-135) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-132) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-128) / 1000);
    }
    if(reception->getLoRaSF() == 12)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-137) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-135) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-129) / 1000);
    }
    return sensitivity;
}

}
