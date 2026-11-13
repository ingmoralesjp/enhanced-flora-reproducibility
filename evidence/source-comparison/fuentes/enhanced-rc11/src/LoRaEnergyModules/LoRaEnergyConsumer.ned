// Copyright (C) 2026
// SPDX-License-Identifier: LGPL-3.0-or-later

package flora.LoRaEnergyModules;

import inet.physicallayer.wireless.common.energyconsumer.StateBasedEpEnergyConsumer;

simple LoRaEnergyConsumer extends StateBasedEpEnergyConsumer
{
    parameters:
        xml configFile;
        // "legacyExactWatts" reproduces the floating-point key lookup;
        // "dbmNearest" uses an integer dBm key with a safe nearest fallback.
        string txCurrentLookupModel = default("dbmNearest");
        @class(LoRaEnergyConsumer);

        // Inherited powerConsumptionChanged carries instantaneous W and is
        // consumed by INET's IdealEpEnergyStorage.
        @signal[totalEnergyConsumption](type=double; unit=J);
        @statistic[totalEnergyConsumed](source=totalEnergyConsumption; record=vector,stats; unit=J; interpolationmode=linear);
        @signal[measurementEnergyConsumption](type=double; unit=J);
        @statistic[measurementEnergyConsumed](source=measurementEnergyConsumption; record=vector,stats; unit=J; interpolationmode=linear);
}
