#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/TimeSystem.hpp"
#include "economy/ContractManager.hpp"
#include "hardware/ServerRoom.hpp"
#include "netrunning/Cyberdeck.hpp"
#include "netrunning/GridMap.hpp"

namespace neon::core {

using nlohmann::json;

constexpr int SAVE_VERSION = 3;

// A hired employee.
struct Employee {
  std::string uid;
  std::string name;
  std::string role; // maintenance, netrunner, manager, security
  int skill = 1;    // 1..5
  double salary_weekly = 0.0;
  int morale = 80;  // 0..100
  std::string assigned_task;

  json to_json() const;
  static Employee from_json(const json& j);
};

// A server the player hacked and claimed (passive income, proxy, heat risk).
struct RemoteServer {
  std::string id;
  std::string name;
  std::string kind; // abandoned, corporate, faction
  double income_per_hour = 0.0;
  double heat_risk = 0.0;
  bool proxy = true;

  json to_json() const;
  static RemoteServer from_json(const json& j);
};

// The whole serializable game state.
struct GameState {
  int version = SAVE_VERSION;
  std::uint64_t seed = 1;
  std::uint64_t rng_state = 0;
  TimeSystem time;

  double credits = 10000.0;
  double rp = 25.0;          // starting RP (per design: enough for first research)
  double data_shards = 0.0;
  int heat = 0;              // 0..100
  int reputation = 0;

  std::map<std::string, hardware::Room> rooms;
  std::string active_room = "room1";
  std::vector<hardware::Component> inventory; // uninstalled components

  std::set<std::string> purchased_tech;
  std::set<std::string> unlocked_items;    // hardware shop unlocks
  std::vector<std::string> unlocked_programs;
  std::set<std::string> unlocked_contract_types = {"courier", "storage", "hosting"};

  economy::ContractManager contracts;

  std::map<std::string, int> faction_rep;
  std::map<std::string, double> market_prices;
  std::map<std::string, int> inventory_count; // consumables / tradables owned

  std::vector<Employee> employees;
  std::vector<RemoteServer> remote_servers;
  std::vector<netrunning::NetrunTarget> netrun_targets;

  netrunning::Cyberdeck cyberdeck;
  std::map<std::string, bool> cyberware;

  // aggregated stat modifiers from tech tree effects: name -> additive bonus
  std::map<std::string, double> stat_bonuses;
  // name -> multiplier
  std::map<std::string, double> stat_multipliers;
  double passive_income_per_day = 0.0;

  // bookkeeping
  std::uint64_t last_autosave_day = 0;
  std::uint64_t last_offer_refresh_day = 0;
  std::uint64_t last_market_update_day = 0;
  std::uint64_t last_event_roll_minute = 0;
  double crypto_amount = 0.0; // mined crypto held
  int total_earned = 0;
  int contracts_completed = 0;
  int netruns_won = 0;

  json to_json() const;
  static GameState from_json(const json& j);
};

} // namespace neon::core
