#pragma once

#include <string>

#include "hardware/HardwareComponent.hpp"

namespace neon::core {
class DataCatalog;
}

namespace neon::hardware {

struct Room;

// Aggregated performance of a server unit (sum of installed parts + base chassis).
struct ServerStats {
  double ghz = 0.0;
  double ram_gb = 0.0;
  double storage_tb = 0.0;
  double mbps = 0.0;        // bandwidth from NICs
  double hash_mh = 0.0;     // mining hash rate from GPUs/ASICs
  double wattage = 0.0;     // PSU capacity
  double cooling_w = 0.0;   // fan cooling
  double power_w = 0.0;     // actual draw
  double heat_w = 0.0;      // heat output
  int compute_units = 0;    // generic "hosting capacity"
  bool has_cpu = false;
  bool has_ram = false;
  bool has_storage = false;
  bool has_psu = false;
};

ServerStats compute_server_stats(const core::DataCatalog& catalog, const Room& room, const Component& server);

// True when the server has power + network cable + minimum parts to run.
bool server_is_runnable(const core::DataCatalog& catalog, const Room& room, const Component& server);

// Effective performance with temperature penalty applied (0..1 factor).
double temperature_performance_factor(double temperature_c);

} // namespace neon::hardware
