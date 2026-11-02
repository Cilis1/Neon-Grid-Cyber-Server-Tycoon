#include "netrunning/Cyberdeck.hpp"

namespace neon::netrunning {

json Cyberdeck::to_json() const {
  return json{{"ram", ram}, {"cpu_speed", cpu_speed}, {"shield", shield}, {"max_shield", max_shield}, {"program_slots", program_slots}};
}

Cyberdeck Cyberdeck::from_json(const json& j) {
  Cyberdeck d;
  d.ram = j.value("ram", 8);
  d.cpu_speed = j.value("cpu_speed", 1.0);
  d.shield = j.value("shield", 100);
  d.max_shield = j.value("max_shield", 100);
  d.program_slots = j.value("program_slots", 4);
  return d;
}

} // namespace neon::netrunning
