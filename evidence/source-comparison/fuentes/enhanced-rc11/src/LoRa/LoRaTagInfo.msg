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

import inet.common.INETDefs;
import inet.common.TagBase;
import inet.common.Units;
import inet.linklayer.common.MacAddress;

cplusplus {{
using namespace inet;
}}

namespace flora;

class LoRaTag extends inet::TagBase
{
    inet::Hz centerFrequency = MHz(812);//que onda con esta frecuencia??? probar con el debug que freceuncia esta tomando
    //comprobar el uso de cabales
    inet::Hz bandwidth = kHz(125);
    int spreadFactor = 12;
    inet::W power = mW(100);
    bool UseHeader = true;
    int codeRendundance = 1;
    bool isGeneratedByGateway = false; // Added this line to know is msg is from GW with channel status
    int packetCounter=0;// Added this line to know numero de msj enviados
    bool isHeartbeat=false;// Added this line to know is msg is heartbeat 
    
}