#include "netrunning/Program.hpp"

namespace neon::netrunning {

ProgramDef ProgramDef::from_json(const json& j) {
  ProgramDef p;
  p.id = j.at("id").get<std::string>();
  p.name = j.value("name", p.id);
  p.description = j.value("description", "");
  p.ram_cost = j.value("ram_cost", 1);
  p.cooldown = j.value("cooldown", 0);
  p.effect_type = j.value("effect_type", "");
  p.effect_value = j.value("effect_value", 0.0);
  p.rarity = j.value("rarity", "common");
  p.lua_script = j.value("lua_script", "");
  return p;
}

json ProgramDef::to_json() const {
  return json{{"id", id}, {"name", name}, {"description", description}, {"ram_cost", ram_cost},
              {"cooldown", cooldown}, {"effect_type", effect_type}, {"effect_value", effect_value},
              {"rarity", rarity}, {"lua_script", lua_script}};
}

} // namespace neon::netrunning
