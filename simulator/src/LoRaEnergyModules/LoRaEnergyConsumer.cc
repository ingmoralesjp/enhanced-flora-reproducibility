// Copyright (C) 2026
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LoRaEnergyConsumer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "inet/common/INETMath.h"

#include "LoRa/LoRaRadio.h"

namespace flora {

using namespace inet;
using namespace inet::physicallayer;
using namespace omnetpp;

Define_Module(LoRaEnergyConsumer);

void LoRaEnergyConsumer::initialize(int stage)
{
    // The energy source must see the correct initial power when it registers
    // this consumer at INITSTAGE_POWER.
    if (stage == INITSTAGE_POWER) {
        previousPower = computePowerBreakdown();
        powerConsumption = previousPower.total();
    }

    StateBasedEpEnergyConsumer::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        txCurrentLookupModel = par("txCurrentLookupModel").stdstringValue();
        if (txCurrentLookupModel != "dbmNearest" &&
                txCurrentLookupModel != "legacyExactWatts")
            throw cRuntimeError("txCurrentLookupModel must be dbmNearest or legacyExactWatts");
        if (!readConfigurationFile())
            throw cRuntimeError("LoRaEnergyConsumer: cannot read configFile");

        // V*mA equals mW. These assignments also document the electrical
        // model exposed through the inherited INET state-based parameters.
        sleepPowerConsumption = mW(supplyVoltage * sleepSupplyCurrent);
        receiverIdlePowerConsumption = mW(supplyVoltage * idleSupplyCurrent);
        transmitterIdlePowerConsumption = mW(supplyVoltage * idleSupplyCurrent);
        receiverReceivingPowerConsumption = mW(supplyVoltage * receiverReceivingSupplyCurrent);
        receiverReceivingPreamblePowerConsumption = receiverReceivingPowerConsumption;
        receiverReceivingHeaderPowerConsumption = receiverReceivingPowerConsumption;
        receiverReceivingDataPowerConsumption = receiverReceivingPowerConsumption;
        receiverBusyPowerConsumption = mW(supplyVoltage * receiverBusySupplyCurrent);

        totalEnergyConsumedSignal = registerSignal("totalEnergyConsumption");
        measurementEnergyConsumedSignal = registerSignal("measurementEnergyConsumption");
        lastEnergyUpdate = simTime();
        previousPower = PowerBreakdown();
    }
}

bool LoRaEnergyConsumer::readConfigurationFile()
{
    cXMLElement *root = par("configFile").xmlValue();
    if (root == nullptr)
        return false;

    auto readRequiredValue = [root](const char *tagName) {
        const auto elements = root->getElementsByTagName(tagName);
        if (elements.empty() || elements.front()->getAttribute("value") == nullptr)
            throw cRuntimeError("%s not defined in energy configuration", tagName);
        return std::strtod(elements.front()->getAttribute("value"), nullptr);
    };

    supplyVoltage = readRequiredValue("supplyVoltage");
    receiverReceivingSupplyCurrent = readRequiredValue("receiverReceivingSupplyCurrent");
    receiverBusySupplyCurrent = readRequiredValue("receiverBusySupplyCurrent");
    idleSupplyCurrent = readRequiredValue("idleSupplyCurrent");
    sleepSupplyCurrent = readRequiredValue("sleepSupplyCurrent");

    const auto containers = root->getElementsByTagName("txSupplyCurrents");
    if (containers.empty())
        throw cRuntimeError("txSupplyCurrents not defined in energy configuration");

    txSupplyCurrentByDbm.clear();
    txSupplyCurrentByWatts.clear();
    for (auto element : containers.front()->getElementsByTagName("txSupplyCurrent")) {
        const char *powerText = element->getAttribute("txPower");
        const char *currentText = element->getAttribute("supplyCurrent");
        if (powerText == nullptr || currentText == nullptr)
            throw cRuntimeError("Each txSupplyCurrent needs txPower and supplyCurrent");
        const double powerDbm = std::strtod(powerText, nullptr);
        const double supplyCurrentMa = std::strtod(currentText, nullptr);
        txSupplyCurrentByDbm[std::lround(powerDbm)] = supplyCurrentMa;
        txSupplyCurrentByWatts[math::dBmW2mW(powerDbm) / 1000.0] = supplyCurrentMa;
    }
    if (txSupplyCurrentByDbm.empty())
        throw cRuntimeError("No TX supply currents defined");
    return true;
}

double LoRaEnergyConsumer::getTxSupplyCurrent(int txPowerDbm) const
{
    auto exact = txSupplyCurrentByDbm.find(txPowerDbm);
    if (exact != txSupplyCurrentByDbm.end())
        return exact->second;

    // A non-tabulated power is mapped to the nearest measured current instead
    // of dereferencing an invalid map iterator or comparing floating watts.
    auto upper = txSupplyCurrentByDbm.lower_bound(txPowerDbm);
    if (upper == txSupplyCurrentByDbm.begin())
        return upper->second;
    if (upper == txSupplyCurrentByDbm.end())
        return std::prev(upper)->second;
    auto lower = std::prev(upper);
    return txPowerDbm - lower->first <= upper->first - txPowerDbm
            ? lower->second : upper->second;
}

double LoRaEnergyConsumer::getLegacyTxSupplyCurrent(double txPowerWatts) const
{
    const auto exact = txSupplyCurrentByWatts.find(txPowerWatts);
    if (exact == txSupplyCurrentByWatts.end())
        throw cRuntimeError("Legacy exact-watt TX-current lookup has no entry for %.17g W",
                txPowerWatts);
    return exact->second;
}

LoRaEnergyConsumer::PowerBreakdown LoRaEnergyConsumer::computePowerBreakdown() const
{
    PowerBreakdown result;
    const IRadio::RadioMode mode = radio->getRadioMode();

    if (mode == IRadio::RADIO_MODE_OFF) {
        result.other = offPowerConsumption;
        return result;
    }
    if (mode == IRadio::RADIO_MODE_SLEEP) {
        result.sleep = sleepPowerConsumption;
        return result;
    }
    if (mode == IRadio::RADIO_MODE_SWITCHING) {
        result.other = switchingPowerConsumption;
        return result;
    }

    if (mode == IRadio::RADIO_MODE_RECEIVER || mode == IRadio::RADIO_MODE_TRANSCEIVER) {
        const auto state = radio->getReceptionState();
        if (state == IRadio::RECEPTION_STATE_IDLE)
            result.idle += receiverIdlePowerConsumption;
        else if (state == IRadio::RECEPTION_STATE_RECEIVING)
            result.rx += receiverReceivingPowerConsumption;
        else if (state == IRadio::RECEPTION_STATE_BUSY)
            result.rx += receiverBusyPowerConsumption;
        else if (state != IRadio::RECEPTION_STATE_UNDEFINED)
            throw cRuntimeError("Unknown LoRa reception state %d", state);
    }

    if (mode == IRadio::RADIO_MODE_TRANSMITTER || mode == IRadio::RADIO_MODE_TRANSCEIVER) {
        const auto state = radio->getTransmissionState();
        if (state == IRadio::TRANSMISSION_STATE_IDLE)
            result.idle += transmitterIdlePowerConsumption;
        else if (state == IRadio::TRANSMISSION_STATE_TRANSMITTING) {
            const auto loraRadio = check_and_cast<const LoRaRadio *>(radio);
            // currentTxPower is stored in W; W*1000 converts it to mW before dBm.
            const double txPowerWatts = loraRadio->getCurrentTxPower();
            const double supplyCurrentMa = txCurrentLookupModel == "legacyExactWatts" ?
                    getLegacyTxSupplyCurrent(txPowerWatts) :
                    getTxSupplyCurrent(std::lround(math::mW2dBmW(txPowerWatts * 1000.0)));
            result.tx += mW(supplyVoltage * supplyCurrentMa);
        }
        else if (state != IRadio::TRANSMISSION_STATE_UNDEFINED)
            throw cRuntimeError("Unknown LoRa transmission state %d", state);
    }
    return result;
}

W LoRaEnergyConsumer::computePowerConsumption() const
{
    return computePowerBreakdown().total();
}

void LoRaEnergyConsumer::integrateEnergyUntil(simtime_t now)
{
    if (now <= lastEnergyUpdate)
        return;

    const double completeSeconds = (now - lastEnergyUpdate).dbl();
    auto accumulate = [](EnergyBreakdown& energy, const PowerBreakdown& power, double seconds) {
        energy.tx += power.tx * s(seconds);
        energy.rx += power.rx * s(seconds);
        energy.idle += power.idle * s(seconds);
        energy.sleep += power.sleep * s(seconds);
        energy.other += power.other * s(seconds);
    };
    accumulate(completeRunEnergy, previousPower, completeSeconds);

    // PDR and application counters exclude warm-up. This second accumulator
    // clips the interval at warm-up so energy/PDR comparisons use one window.
    const simtime_t warmup = getSimulation()->getWarmupPeriod();
    const simtime_t measuredStart = std::max(lastEnergyUpdate, warmup);
    if (now > measuredStart)
        accumulate(measurementEnergy, previousPower, (now - measuredStart).dbl());

    lastEnergyUpdate = now;
    emit(totalEnergyConsumedSignal, completeRunEnergy.total().get());
    emit(measurementEnergyConsumedSignal, measurementEnergy.total().get());
}

void LoRaEnergyConsumer::receiveSignal(cComponent *source, simsignal_t signal,
        intval_t value, cObject *details)
{
    // Integrate the old state up to the exact state-change timestamp, then let
    // INET update and announce the new instantaneous power in watts.
    integrateEnergyUntil(simTime());
    StateBasedEpEnergyConsumer::receiveSignal(source, signal, value, details);
    previousPower = computePowerBreakdown();
}

void LoRaEnergyConsumer::finish()
{
    // Account for the final interval, which has no following state-change signal.
    integrateEnergyUntil(simTime());

    recordScalar("totalEnergyConsumed", completeRunEnergy.total().get(), "J");
    recordScalar("measurementEnergyConsumed", measurementEnergy.total().get(), "J");
    // Component values over the same post-warm-up window as PDR. These avoid
    // comparing a 10-day delivery population with a 12-day TX/RX/sleep total.
    recordScalar("measurementTxEnergyConsumed", measurementEnergy.tx.get(), "J");
    recordScalar("measurementRxEnergyConsumed", measurementEnergy.rx.get(), "J");
    recordScalar("measurementIdleEnergyConsumed", measurementEnergy.idle.get(), "J");
    recordScalar("measurementSleepEnergyConsumed", measurementEnergy.sleep.get(), "J");
    recordScalar("measurementOtherEnergyConsumed", measurementEnergy.other.get(), "J");
    recordScalar("txEnergyConsumed", completeRunEnergy.tx.get(), "J");
    recordScalar("rxEnergyConsumed", completeRunEnergy.rx.get(), "J");
    recordScalar("idleEnergyConsumed", completeRunEnergy.idle.get(), "J");
    recordScalar("sleepEnergyConsumed", completeRunEnergy.sleep.get(), "J");
    recordScalar("otherEnergyConsumed", completeRunEnergy.other.get(), "J");
    recordScalar("finalPowerConsumption", powerConsumption.get(), "W");
    recordScalar("txCurrentLookupModelCode", txCurrentLookupModel == "dbmNearest" ? 1 : 0);
}

} // namespace flora
