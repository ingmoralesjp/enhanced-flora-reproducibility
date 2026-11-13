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

//
// TODO generated message class
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace flora;

enum AppPacketType
{
    JOIN_REQUEST = 1;
    JOIN_REPLY = 2;
    DATA = 3;
    TXCONFIG = 4;
    ACK = 5;
}

// Classification produced by the adaptive mobility-aware ADR at the server.
enum AdrMobilityClass
{
    ADR_MOBILITY_UNKNOWN = 0;
    ADR_MOBILITY_STATIC = 1;
    ADR_MOBILITY_MOBILE = 2;
}

class LoRaOptions 
{
    double LoRaTP = -1;
    double LoRaCF = -1;
    int LoRaSF = -1;
    double LoRaBW = -1;
    int LoRaCR = -1;
    bool UseHeader = true;
    bool ADRACKReq = false;
    int packetCounter = 0;
    double distanceED = 0;
    // Position is already assumed by the thesis' distance-based ADR. Sending
    // both coordinates lets the server distinguish displacement from fading.
    double positionX = 0;
    double positionY = 0;
    bool hasPosition = false;
    simtime_t sentAt = 0;
    bool isHeartbeat = false;
    bool isconfirmedMSG = false;
    // Downlink-only adaptive controls. -1 means "leave unchanged".
    double heartbeatInterval = -1;
    int mobilityClass @enum(AdrMobilityClass) = -1;
    // Observed uplink SNR returned by the MADERE reconstruction.
    double feedbackSnr = -999;
}

class LoRaAppPacket extends inet::FieldsChunk {
    int msgType @enum(AppPacketType);
    int sampleMeasurement;
    LoRaOptions options;
}
