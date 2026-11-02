#include "economy/Client.hpp"

#include "core/Game.hpp"
#include "utils/Random.hpp"
#include "utils/StringUtils.hpp"

namespace neon::economy {

Client Client::generate(neon::core::Game& game, const std::string& faction_id) {
  static const char* corps[] = {"Arasoft", "Kurosawa Heavy", "Helix Biotech", "Vantacore", "Omninet"};
  static const char* gangs[] = {"Neon Serpents", "Red Circuit", "The Static", "Chrome Wolves"};
  static const char* brokers[] = {"Data Wraith", "Infohazard", "The Broker", "Ghostline"};
  Client c;
  auto& rng = game.rng();
  c.id = "client_" + std::to_string(rng.next_u64() % 100000);
  c.faction_id = faction_id;
  const char** pool = brokers;
  int pool_n = 4;
  if (faction_id == "arasaka" || faction_id == "arasaka_corp" || faction_id == "corp") {
    pool = corps;
    pool_n = 5;
  } else if (faction_id == "neon_serpents" || faction_id == "gang") {
    pool = gangs;
    pool_n = 4;
  }
  c.name = std::string(pool[rng.next_int(0, pool_n - 1)]) + " " + utils::random_handle(rng.next_u64(), "rep");
  c.handle = "@" + utils::to_lower(utils::random_handle(rng.next_u64(), "x"));
  return c;
}

json Client::to_json() const {
  return json{{"id", id}, {"name", name}, {"faction_id", faction_id}, {"handle", handle}};
}

Client Client::from_json(const json& j) {
  Client c;
  c.id = j.at("id").get<std::string>();
  c.name = j.value("name", c.id);
  c.faction_id = j.value("faction_id", "");
  c.handle = j.value("handle", "");
  return c;
}

} // namespace neon::economy
