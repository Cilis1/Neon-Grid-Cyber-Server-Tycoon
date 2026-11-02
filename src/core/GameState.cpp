#include "core/GameState.hpp"

namespace neon::core {

json Employee::to_json() const {
  return json{{"uid", uid}, {"name", name}, {"role", role}, {"skill", skill},
              {"salary_weekly", salary_weekly}, {"morale", morale}, {"assigned_task", assigned_task}};
}

Employee Employee::from_json(const json& j) {
  Employee e;
  e.uid = j.at("uid").get<std::string>();
  e.name = j.value("name", e.uid);
  e.role = j.value("role", "maintenance");
  e.skill = j.value("skill", 1);
  e.salary_weekly = j.value("salary_weekly", 0.0);
  e.morale = j.value("morale", 80);
  e.assigned_task = j.value("assigned_task", "");
  return e;
}

json RemoteServer::to_json() const {
  return json{{"id", id}, {"name", name}, {"kind", kind}, {"income_per_hour", income_per_hour},
              {"heat_risk", heat_risk}, {"proxy", proxy}};
}

RemoteServer RemoteServer::from_json(const json& j) {
  RemoteServer r;
  r.id = j.at("id").get<std::string>();
  r.name = j.value("name", r.id);
  r.kind = j.value("kind", "abandoned");
  r.income_per_hour = j.value("income_per_hour", 0.0);
  r.heat_risk = j.value("heat_risk", 0.0);
  r.proxy = j.value("proxy", true);
  return r;
}

json GameState::to_json() const {
  json rooms_j = json::object();
  for (const auto& [id, room] : rooms) rooms_j[id] = room.to_json();
  json inventory_j = json::array();
  for (const auto& c : inventory) inventory_j.push_back(c.to_json());
  json targets_j = json::array();
  for (const auto& t : netrun_targets) targets_j.push_back(t.to_json());
  json employees_j = json::array();
  for (const auto& e : employees) employees_j.push_back(e.to_json());
  json remotes_j = json::array();
  for (const auto& r : remote_servers) remotes_j.push_back(r.to_json());
  json purchased = json::array();
  for (const auto& p : purchased_tech) purchased.push_back(p);
  json unlocked_items_j = json::array();
  for (const auto& u : unlocked_items) unlocked_items_j.push_back(u);
  json programs_j = json::array();
  for (const auto& p : unlocked_programs) programs_j.push_back(p);
  json contract_types = json::array();
  for (const auto& t : unlocked_contract_types) contract_types.push_back(t);

  return json{
      {"version", version}, {"seed", seed}, {"rng_state", rng_state},
      {"minute", time.minute},
      {"credits", credits}, {"rp", rp}, {"data_shards", data_shards},
      {"heat", heat}, {"reputation", reputation},
      {"rooms", rooms_j}, {"active_room", active_room},
      {"inventory", inventory_j},
      {"purchased_tech", purchased},
      {"unlocked_items", unlocked_items_j},
      {"unlocked_programs", programs_j},
      {"unlocked_contract_types", contract_types},
      {"contracts", contracts.to_json()},
      {"faction_rep", faction_rep},
      {"market_prices", market_prices},
      {"inventory_count", inventory_count},
      {"employees", employees_j},
      {"remote_servers", remotes_j},
      {"netrun_targets", targets_j},
      {"cyberdeck", cyberdeck.to_json()},
      {"cyberware", cyberware},
      {"stat_bonuses", stat_bonuses},
      {"stat_multipliers", stat_multipliers},
      {"passive_income_per_day", passive_income_per_day},
      {"last_autosave_day", last_autosave_day},
      {"last_offer_refresh_day", last_offer_refresh_day},
      {"last_market_update_day", last_market_update_day},
      {"last_event_roll_minute", last_event_roll_minute},
      {"crypto_amount", crypto_amount},
      {"total_earned", total_earned},
      {"contracts_completed", contracts_completed},
      {"netruns_won", netruns_won},
  };
}

GameState GameState::from_json(const json& j) {
  GameState s;
  s.version = j.value("version", 1);
  s.seed = j.value("seed", 1ULL);
  s.rng_state = j.value("rng_state", 0ULL);
  s.time.minute = j.value("minute", 0ULL);
  s.credits = j.value("credits", 10000.0);
  s.rp = j.value("rp", 0.0);
  s.data_shards = j.value("data_shards", 0.0);
  s.heat = j.value("heat", 0);
  s.reputation = j.value("reputation", 0);
  if (j.contains("rooms") && j["rooms"].is_object())
    for (auto& [id, r] : j["rooms"].items()) s.rooms[id] = hardware::Room::from_json(r);
  s.active_room = j.value("active_room", "room1");
  if (j.contains("inventory") && j["inventory"].is_array())
    for (auto& c : j["inventory"]) s.inventory.push_back(hardware::Component::from_json(c));
  if (j.contains("purchased_tech") && j["purchased_tech"].is_array())
    for (auto& p : j["purchased_tech"]) s.purchased_tech.insert(p.get<std::string>());
  if (j.contains("unlocked_items") && j["unlocked_items"].is_array())
    for (auto& u : j["unlocked_items"]) s.unlocked_items.insert(u.get<std::string>());
  if (j.contains("unlocked_programs") && j["unlocked_programs"].is_array())
    for (auto& p : j["unlocked_programs"]) s.unlocked_programs.push_back(p.get<std::string>());
  if (j.contains("unlocked_contract_types") && j["unlocked_contract_types"].is_array())
    for (auto& t : j["unlocked_contract_types"]) s.unlocked_contract_types.insert(t.get<std::string>());
  if (j.contains("contracts")) s.contracts.from_json(j["contracts"]);
  if (j.contains("faction_rep") && j["faction_rep"].is_object())
    for (auto& [k, v] : j["faction_rep"].items()) s.faction_rep[k] = v.get<int>();
  if (j.contains("market_prices") && j["market_prices"].is_object())
    for (auto& [k, v] : j["market_prices"].items()) s.market_prices[k] = v.get<double>();
  if (j.contains("inventory_count") && j["inventory_count"].is_object())
    for (auto& [k, v] : j["inventory_count"].items()) s.inventory_count[k] = v.get<int>();
  if (j.contains("employees") && j["employees"].is_array())
    for (auto& e : j["employees"]) s.employees.push_back(Employee::from_json(e));
  if (j.contains("remote_servers") && j["remote_servers"].is_array())
    for (auto& r : j["remote_servers"]) s.remote_servers.push_back(RemoteServer::from_json(r));
  if (j.contains("netrun_targets") && j["netrun_targets"].is_array())
    for (auto& t : j["netrun_targets"]) s.netrun_targets.push_back(netrunning::NetrunTarget::from_json(t));
  if (j.contains("cyberdeck")) s.cyberdeck = netrunning::Cyberdeck::from_json(j["cyberdeck"]);
  if (j.contains("cyberware") && j["cyberware"].is_object())
    for (auto& [k, v] : j["cyberware"].items()) s.cyberware[k] = v.get<bool>();
  if (j.contains("stat_bonuses") && j["stat_bonuses"].is_object())
    for (auto& [k, v] : j["stat_bonuses"].items()) s.stat_bonuses[k] = v.get<double>();
  if (j.contains("stat_multipliers") && j["stat_multipliers"].is_object())
    for (auto& [k, v] : j["stat_multipliers"].items()) s.stat_multipliers[k] = v.get<double>();
  s.passive_income_per_day = j.value("passive_income_per_day", 0.0);
  s.last_autosave_day = j.value("last_autosave_day", 0ULL);
  s.last_offer_refresh_day = j.value("last_offer_refresh_day", 0ULL);
  s.last_market_update_day = j.value("last_market_update_day", 0ULL);
  s.last_event_roll_minute = j.value("last_event_roll_minute", 0ULL);
  s.crypto_amount = j.value("crypto_amount", 0.0);
  s.total_earned = j.value("total_earned", 0);
  s.contracts_completed = j.value("contracts_completed", 0);
  s.netruns_won = j.value("netruns_won", 0);
  return s;
}

} // namespace neon::core
