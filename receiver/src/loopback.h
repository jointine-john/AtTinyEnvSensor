// Test mode (build the "loopback" environment): transmits fake sensor packets
// with every reading type (T/H, pressure, wind, rain) on GPIO5, using exactly
// the ATtiny85's encoding, so the receiver can be checked without a sensor node.
//
// Jumper GPIO5 -> GPIO4 to test the decoder, MQTT and Home Assistant, or wire
// GPIO5 to an STX882's DATA pin to test over the air.
#pragma once

void loopbackBegin();
void loopbackLoop();
