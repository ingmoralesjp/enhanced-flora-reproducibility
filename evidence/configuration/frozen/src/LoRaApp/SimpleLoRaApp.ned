package flora.LoRaApp;

simple SimpleLoRaApp
{
    parameters:
        @signal[LoRa_AppPacketSent](type=long);
        @statistic[LoRa_AppPacketSent](source=LoRa_AppPacketSent; record=count);
        @signal[CambiosSF](type=long);
        @statistic[CambiosSF](source=CambiosSF; record=vector,stats; interpolationMode=sample-hold);
        @signal[CambiosTP](type=long);
        @statistic[CambiosTP](source=CambiosTP; record=vector,stats; interpolationMode=sample-hold);
        @signal[HeartbeatIntervalChanged](type=double; unit=s);
        @statistic[HeartbeatIntervalChanged](source=HeartbeatIntervalChanged; record=vector,stats; unit=s; interpolationMode=sample-hold);
        @signal[MobilityClassChanged](type=long);
        @statistic[MobilityClassChanged](source=MobilityClassChanged; record=vector,stats; interpolationMode=sample-hold);

        int numberOfPacketsToSend = default(1); // 0 means unlimited
        volatile double timeToFirstPacket @unit(s) = default(10s);
        volatile double timeToNextPacket @unit(s) = default(10s);
        // Initial value. Mobility-aware ADR may later update it by TXCONFIG.
        double heartbeatInterval @unit(s) = default(6h); // 0 disables heartbeat
        // Proposal-only keepalive semantics: ordinary data proves liveness and
        // restarts the heartbeat countdown. Baseline treatments leave it false.
        bool heartbeatOnlyWhenDataSilent = default(false);

        double initialLoRaTP @unit(dBm) = default(14dBm);
        int initialLoRaSF = default(12);
        double initialLoRaBW @unit(Hz) = default(125kHz);
        int initialLoRaCR = default(4);
        bool initialUseHeader = default(true);
        bool randomizeInitialRadioParameters = default(true);

        // false: mandatory EU868 channels; true: eight-channel thesis proposal
        bool useAllEu868Channels = default(true);
        bool evaluateADRinNode = default(false);
        int dataSize @unit(B) = default(10B);
        // M2 carries two IEEE-754 single-precision coordinates. Their bytes
        // are part of PHY airtime, collision exposure and measured TX energy.
        bool includePosition = default(false);
        int positionPayloadBytes @unit(B) = default(8B);
        // Sensor/GNSS acquisition cost is additive to the radio integrator.
        // It remains zero until a sourced hardware value is selected.
        double localizationEnergyPerFix @unit(J) = default(0J);
        // App-side comparison algorithms are projected onto the same packet
        // clock as every other treatment; they never create extra traffic.
        string clientAdrMode = default("network"); // network, blind, hadrMobile, madere
        double madereCellSize @unit(m) = default(300m);
        double madereSnrMarginCells = default(8);
        double madereMaxSpeed @unit(mps) = default(3mps);
        double madereGamma = default(2.32);
        double madereD0 @unit(m) = default(1000m);
        double madereNoiseFigure @unit(dB) = default(6dB);
        double madereFrequency @unit(Hz) = default(868MHz);

    gates:
        output appOut @labels(LoRaAppPacket/down);
        input appIn @labels(LoRaAppPacket/up);
}
