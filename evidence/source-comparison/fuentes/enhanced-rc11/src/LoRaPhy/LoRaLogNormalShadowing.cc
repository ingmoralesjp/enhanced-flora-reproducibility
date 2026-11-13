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

#include "LoRaLogNormalShadowing.h"
#include "inet/common/INETMath.h"

namespace flora {

Define_Module(LoRaLogNormalShadowing);

void LoRaLogNormalShadowing::initialize(int stage)
{
    FreeSpacePathLoss::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        sigma = par("sigma");
        gamma = par("gamma");
        d0 = m(par("d0"));
        useFriisReference = par("useFriisReference");
        configuredPathLossDb = par("referencePathLoss");
        transmitterGainDb = par("transmitterGain");
        receiverGainDb = par("receiverGain");
        if (d0 <= m(0))
            throw cRuntimeError("The reference distance d0 must be positive");

        if (useFriisReference) {
            const double frequency = par("referenceFrequency").doubleValue();
            const double propagationSpeed = par("propagationSpeed").doubleValue();
            const double wavelength = propagationSpeed / frequency;
            referencePathLossDb = 20 * log10(4 * M_PI * d0.get() / wavelength)
                    + 10 * log10(systemLoss) - transmitterGainDb - receiverGainDb;
        }
        else
            referencePathLossDb = configuredPathLossDb;
    }
}

std::ostream& LoRaLogNormalShadowing::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "LoRaLogNormalShadowing";
    if (level <= PRINT_LEVEL_TRACE)
        stream << ", alpha = " << alpha
               << ", systemLoss = " << systemLoss
               << ", sigma = " << sigma;
    return stream;
}

double LoRaLogNormalShadowing::computePathLoss(mps propagationSpeed, Hz frequency, m distance) const
{
    if (frequency <= Hz(0) || propagationSpeed <= mps(0) || distance <= m(0))
        throw cRuntimeError("Propagation speed, carrier frequency and distance must be positive");

    // Friis is used only to anchor PL(d0); gamma and sigma continue to
    // represent large-scale attenuation and environmental shadowing.
    const double shadowingDb = normal(0.0, sigma);
    const double pathLossDb = referencePathLossDb
            + 10 * gamma * log10(unit(distance / d0).get())
            + shadowingDb;

    return math::dB2fraction(-pathLossDb);
}

m LoRaLogNormalShadowing::computeLoRaRange(W transmissionPower) const
{
    // LoRa's SF12 sensitivity at 125 kHz is used as the conservative range
    // boundary expected by LoRaMediumCache.
    constexpr double receiverSensitivityDbm = -137.0;
    const double transmissionPowerDbm = 10 * log10(transmissionPower.get() * 1000);
    const double maximumPathLossDb = transmissionPowerDbm - receiverSensitivityDbm;
    return d0 * pow(10, (maximumPathLossDb - referencePathLossDb) / (10 * gamma));
}

} // namespace flora
