#pragma once
#include <stdint.h>

namespace skb {
// Relative overlap score, not measured channel utilization or airtime.
inline uint32_t overlapScore(int observedChannel, int rssi, int channel) {
  int distance = observedChannel > channel ? observedChannel-channel : channel-observedChannel;
  if (observedChannel < 1 || observedChannel > 14 || channel < 1 || channel > 14 || distance > 4) return 0;
  int strength = rssi + 100;
  if (strength < 1) strength = 1;
  if (strength > 100) strength = 100;
  return static_cast<uint32_t>(strength * (5-distance));
}
}
