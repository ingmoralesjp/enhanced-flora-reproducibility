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

import inet.physicallayer.wireless.common.pathloss.FreeSpacePathLoss;

module LoRaLogNormalShadowing extends FreeSpacePathLoss
{
    parameters:
        // Close-in reference parameters. Urban thesis values are
        // d0=40 m, gamma=2.08, sigma=3.57 dB; suburban values are
        // d0=1000 m, gamma=2.32, sigma=7.08 dB.
        double d0 = default(40m) @unit(m);
        double gamma = default(2.08);
        double sigma = default(3.57);
        bool useFriisReference = default(true);
        double referencePathLoss = default(127.41); // dB, used by legacy FLNS
        double referenceFrequency @unit(Hz) = default(868MHz);
        double propagationSpeed @unit(mps) = default(299792458mps);
        double transmitterGain = default(1); // dBi
        double receiverGain = default(1); // dBi
        @class(LoRaLogNormalShadowing);
}
