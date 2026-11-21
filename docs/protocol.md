# Radio protocol

Defined in [`common/protocol.h`](../common/protocol.h), which both firmwares include.

## Physical layer

- 433.92 MHz OOK (on-off keying)
- Manchester encoding, 1000 bit/s (500 µs half-bits). The first half-bit
  carries the bit value, the second half its inverse.
- MSB first

## Frame

| Field | Bytes | Value |
|---|---|---|
| Preamble | 4 | `0xAA` × 4, lets the receiver's gain settle |
| Sync | 1 | `0x2D` |
| Payload | 8 | See below |
| CRC | 1 | CRC-8, polynomial 0x31, init 0xFF, over the payload |
| Trailer | 1 bit | `0`, so the last CRC half-bit ends on an edge |

## Payload (big-endian)

| Offset | Type | Field |
|---|---|---|
| 0 | uint8 | Node ID |
| 1 | uint8 | Sequence number (wraps) |
| 2–3 | int16 | Temperature, centi-°C (`0x7FFF` = sensor error) |
| 4–5 | uint16 | Relative humidity, centi-% |
| 6–7 | uint16 | Battery voltage, mV |

## Reliability

There is no acknowledgement. Instead:

- Each frame is sent 3 times, 20 ms apart. The receiver de-duplicates on
  (node, seq) within 5 seconds.
- Nodes randomly add 8 s to some intervals so sensors that start together
  drift apart instead of colliding every time.
- The decoder runs two alignments in parallel and resynchronises on any pulse
  that isn't one or two half-bits long, so noise between packets is ignored.

## Regulatory

433.92 MHz at well under 1% duty cycle is within UK/EU SRD (IR2030) limits.
