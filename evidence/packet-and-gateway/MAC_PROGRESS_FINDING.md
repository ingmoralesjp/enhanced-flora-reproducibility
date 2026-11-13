# Item 2 finding — native MAC timeout consumption and terminal queues

The nine instrumented probes reconcile with the released scalar definitions.
They also expose a native MAC liveness failure. A short replay uses the
historical library itself (SHA-256
`a83fbfd8ee4c9557fd1b0491fb390ceaf880eeed1bd7cbd4d95108cee4bbfcb5`),
the same F2 static configuration, run 0 and seed-set, with a 3000-s horizon
and native log output enabled for ED 1. No MAC implementation is changed.

## Recorded example

| Time (s) | Native MAC observation |
| ---: | --- |
| 2902.80938567681 | Uplink completes; RX-window timers are scheduled. |
| 2903.80938567681 | `Delay_1` arrives in `WAIT_DELAY_1`. |
| 2904.01377121952 | Radio state change takes `LISTENING_1` to `RECEIVING_1`. |
| 2904.80938567681 | `Listening_1` expiry arrives while still in `RECEIVING_1`. |
| 2905.80938567681 | `Delay_2` expiry also arrives in `RECEIVING_1`. |
| 2905.98804321952 | `Dropped Packet` arrives in `RECEIVING_1`; that branch returns to `LISTENING_1`. |
| 2906.80938567681 | `Listening_2` expiry arrives in `LISTENING_1`. |
| 2967.48924943833 | New application packet is deferred in `WAIT_DELAY_2`. |

The full-horizon trace continues to show admissions in `WAIT_DELAY_2` through
day 12. ED 1 has 1047 pending packets from all generations at the end,
including 868 generated during days 2--12. Its oldest pending generation is
at 2967.48924943833 s. These are queued packets, not observed RF losses.

## Source interpretation

`LoRaMac.cc` uses `FSMA_Event_Transition` for the timed listening/wait states.
INET's `FSMA.h` consumes the current event when one such transition fires;
entering the target state does not immediately execute another event
transition. The expiry checks include `!timer->isScheduled()`, but they are
still event transitions rather than no-event catch-up transitions.

In the recorded sequence, RX1 close and RX2 open are consumed while the MAC
is receiving. After that reception fails, the RX2-close event is handled in
the returned `LISTENING_1` state. The expired RX1 timer permits a transition
to `WAIT_DELAY_2`; the already-expired RX2-open timer does not produce a new
event there. Later application arrivals only enqueue while the FSM is not
IDLE. The logs, source transition definitions and terminal queue therefore
identify a reproducible loss of MAC progress in this tested path.

## Consequence for the measurements

The released PDR is delivered/application-generated, so its denominator
includes untransmitted queued packets. Changing airtime and downlink timing
can change exposure to this timeout sequence. Energy can decrease when fewer
queued packets reach transmission and RX-window activity, even while energy
per successful delivery increases. It is not defensible to attribute the
entire observed adjacent-stage change to propagation, collisions or physical
airtime alone.

The item-2 report preserves the original model and exposes the pending
population and residues. Item 3 must label its effects as effects of the
specified factors on this implementation and retain queue-progress outcomes.
The timeout path needs targeted regression and a separately versioned
correction before any claim that the revised MAC maintains progress under
all these window overlaps. Such a correction must not be silently mixed
into the historical 900-run evidence.

## Files and limitations

The supplement contains the replay script, native log and selected timer
events, scalar output, command receipt, original MAC source and INET macro
header. `timeline.csv` is an extract from the full run; it explicitly labels
deadlines derived from source separately from native observed events. The
native replay provides the actual timer callbacks for this example.

The earlier replay setup attempts completed or failed before useful per-MAC
logging was enabled; their files remain in the working analysis directory.
The accepted diagnostic is `mac-progress-diagnostic-v4`. Those setup attempts
are not campaign failures and are not substituted for the nine accepted
full-horizon probes. This case confirms the demonstrated path, not that
every queued node reached it by an identical packet sequence.
