// Copyright (C) 2026
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef LORAPHY_LORAAIRTIMECALCULATOR_H_
#define LORAPHY_LORAAIRTIMECALCULATOR_H_

#include <algorithm>
#include <cmath>

#include <omnetpp.h>

namespace flora {

/** Durations of the LoRa PHY parts; their sum is the complete time on air. */
struct LoRaAirtime
{
    omnetpp::simtime_t preamble;
    omnetpp::simtime_t header;
    omnetpp::simtime_t payload;

    omnetpp::simtime_t total() const { return preamble + header + payload; }
};

/**
 * Shared Semtech-compatible LoRa time-on-air calculation.
 *
 * Keeping this formula in one place ensures that the physical transmitter and
 * the energy-aware ADR optimizer evaluate exactly the same packet duration.
 */
inline LoRaAirtime calculateLoRaAirtime(int spreadingFactor, double bandwidthHz,
        int payloadBytes, int codeRedundance, bool useHeader)
{
    if (spreadingFactor < 7 || spreadingFactor > 12)
        throw omnetpp::cRuntimeError("Unsupported LoRa spreading factor SF%d", spreadingFactor);
    if (bandwidthHz <= 0)
        throw omnetpp::cRuntimeError("LoRa bandwidth must be positive");
    if (payloadBytes < 0)
        throw omnetpp::cRuntimeError("LoRa PHY payload length cannot be negative");
    if (codeRedundance < 1 || codeRedundance > 4)
        throw omnetpp::cRuntimeError("LoRa code redundancy must be in [1,4], got %d", codeRedundance);

    // A LoRa symbol lasts 2^SF/BW. DE is enabled for symbol durations of at
    // least 16 ms (SF11 and SF12 at 125 kHz).
    const double symbolDuration = std::pow(2.0, spreadingFactor) / bandwidthHz;
    const int lowDataRateOptimization = symbolDuration >= 0.016 ? 1 : 0;
    const int implicitHeader = useHeader ? 0 : 1;

    // CRC is enabled. CR=1..4 represents coding rates 4/5..4/8.
    const double numerator = 8.0 * payloadBytes - 4.0 * spreadingFactor + 28.0
            + 16.0 - 20.0 * implicitHeader;
    const double denominator = 4.0 * (spreadingFactor - 2 * lowDataRateOptimization);
    const int codedBlocks = std::max(0, static_cast<int>(std::ceil(numerator / denominator)));
    const int payloadSymbols = 8 + codedBlocks * (codeRedundance + 4);

    // The explicit header occupies the first eight payload symbols. This split
    // supports INET's part scheduling and does not change the total duration.
    const int headerSymbols = useHeader ? std::min(8, payloadSymbols) : 0;
    LoRaAirtime result;
    result.preamble = (8.0 + 4.25) * symbolDuration;
    result.header = headerSymbols * symbolDuration;
    result.payload = (payloadSymbols - headerSymbols) * symbolDuration;
    return result;
}

/**
 * Recovered pre-correction FLoRa timing model.
 *
 * The inherited transmitter used a fixed 20-byte ED payload (15 bytes for a
 * gateway), forced DE=0, and split an extra (8+payloadSymbols) term equally
 * between header and payload. It is retained only as an explicit ablation
 * treatment and must not be used as a LoRa timing reference.
 */
inline LoRaAirtime calculateLegacyLoRaAirtime(int spreadingFactor,
        double bandwidthHz, int fixedPayloadBytes, int codeRedundance)
{
    if (spreadingFactor < 7 || spreadingFactor > 12)
        throw omnetpp::cRuntimeError("Unsupported LoRa spreading factor SF%d", spreadingFactor);
    if (bandwidthHz <= 0)
        throw omnetpp::cRuntimeError("LoRa bandwidth must be positive");
    if (fixedPayloadBytes < 0)
        throw omnetpp::cRuntimeError("Legacy LoRa payload length cannot be negative");
    if (codeRedundance < 1 || codeRedundance > 4)
        throw omnetpp::cRuntimeError("LoRa code redundancy must be in [1,4], got %d", codeRedundance);

    const double symbolDuration = std::pow(2.0, spreadingFactor) / bandwidthHz;
    int payloadSymbols = 8 + static_cast<int>(std::ceil(
            (8.0 * fixedPayloadBytes - 4.0 * spreadingFactor + 28.0 + 16.0) /
            (4.0 * spreadingFactor))) * (codeRedundance + 4);
    payloadSymbols = std::max(8, payloadSymbols);

    LoRaAirtime result;
    result.preamble = (8.0 + 4.25) * symbolDuration;
    result.header = 0.5 * (8 + payloadSymbols) * symbolDuration;
    result.payload = result.header;
    return result;
}

} // namespace flora

#endif
