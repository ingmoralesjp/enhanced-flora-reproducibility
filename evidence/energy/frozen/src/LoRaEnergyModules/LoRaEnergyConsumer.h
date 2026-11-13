// Copyright (C) 2026
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef LORAENERGYMODULES_LORAENERGYCONSUMER_H_
#define LORAENERGYMODULES_LORAENERGYCONSUMER_H_

#include <map>
#include <string>

#include "inet/physicallayer/wireless/common/energyconsumer/StateBasedEpEnergyConsumer.h"

namespace flora {

/**
 * State-based radio consumer used by the thesis.
 *
 * Electrical power is P=V*I. Energy is integrated as E=sum(Pstate*dt)
 * for TX, RX, idle and sleep, with TX current selected by radiated dBm.
 */
class LoRaEnergyConsumer : public inet::physicallayer::StateBasedEpEnergyConsumer
{
  protected:
    struct EnergyBreakdown {
        inet::J tx = inet::J(0);
        inet::J rx = inet::J(0);
        inet::J idle = inet::J(0);
        inet::J sleep = inet::J(0);
        inet::J other = inet::J(0);

        inet::J total() const { return tx + rx + idle + sleep + other; }
    };

    struct PowerBreakdown {
        inet::W tx = inet::W(0);
        inet::W rx = inet::W(0);
        inet::W idle = inet::W(0);
        inet::W sleep = inet::W(0);
        inet::W other = inet::W(0);

        inet::W total() const { return tx + rx + idle + sleep + other; }
    };

    // Currents are read in mA; voltage is read in V.
    double supplyVoltage = 0;
    double receiverReceivingSupplyCurrent = 0;
    double receiverBusySupplyCurrent = 0;
    double idleSupplyCurrent = 0;
    double sleepSupplyCurrent = 0;
    std::map<int, double> txSupplyCurrentByDbm;
    std::map<double, double> txSupplyCurrentByWatts;
    std::string txCurrentLookupModel = "dbmNearest";

    omnetpp::simtime_t lastEnergyUpdate;
    PowerBreakdown previousPower;
    EnergyBreakdown completeRunEnergy;
    EnergyBreakdown measurementEnergy;

    omnetpp::simsignal_t totalEnergyConsumedSignal;
    omnetpp::simsignal_t measurementEnergyConsumedSignal;

    bool readConfigurationFile();
    double getTxSupplyCurrent(int txPowerDbm) const;
    double getLegacyTxSupplyCurrent(double txPowerWatts) const;
    PowerBreakdown computePowerBreakdown() const;
    void integrateEnergyUntil(omnetpp::simtime_t now);

    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual inet::W computePowerConsumption() const override;

  public:
    virtual void receiveSignal(omnetpp::cComponent *source,
            omnetpp::simsignal_t signal, omnetpp::intval_t value,
            omnetpp::cObject *details) override;
};

} // namespace flora

#endif
