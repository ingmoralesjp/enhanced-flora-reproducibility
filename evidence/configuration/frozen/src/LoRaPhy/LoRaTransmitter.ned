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

package flora.LoRaPhy;

import inet.physicallayer.wireless.common.base.packetlevel.NarrowbandTransmitterBase;

module LoRaTransmitter extends NarrowbandTransmitterBase
{
    parameters:
        @signal[LoRaTransmissionCreated](type=bool);
        @statistic[LoRaTransmissionCreated](source=LoRaTransmissionCreated; record=count);
        @signal[timeOnAir](type=simtime_t; unit=s);
        @statistic[timeOnAir](source=timeOnAir; record=vector,stats; unit=s);
        modulation = default("LoRaModulation");
        @class(LoRaTransmitter);
        // "legacyFixedPayload" reproduces the recovered pre-correction FLoRa
        // formula; "semtech" uses actual PHY bytes and DE when required.
        string airtimeModel = default("semtech");
        // Inputs used only by the SF7..SF12 reference scalars in finish().
        // 24 PHY bytes (11 application bytes plus LoRaWAN overhead), CR 4/5 and
        // BW 125 kHz reproduce the thesis table: SF7=61.696 ms, SF12=1482.752 ms.
        int referencePayloadSize @unit(B) = default(24B);
        int referenceCodeRedundance = default(1); // 1..4 means CR 4/5..4/8
        bool referenceUseHeader = default(true);
        double referenceBandwidth @unit(Hz) = default(125kHz);
}
