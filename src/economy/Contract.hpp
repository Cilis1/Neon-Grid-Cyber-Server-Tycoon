#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "economy/Client.hpp"

namespace neon::economy {

using nlohmann::json;

// Static contract template (data/contracts.json).
struct ContractTemplate {
  std::string id;
  std::string type; // courier, storage, hosting, mining, data_broker, botnet
  std::string title;
  std::string description;
  std::string client_faction;
  double reward_min = 0.0;
  double reward_max = 0.0;
  int duration_hours = 24;
  json requirements;   // e.g. {"storage_tb": 10, "bandwidth_mbps": 100, "uptime_pct": 99}
  double risk = 0.0;   // heat increase on completion (illegal jobs)
  json rep_effects;    // {"faction_id": +5}
  int difficulty = 1;

  static ContractTemplate from_json(const json& j);
  json to_json() const;
};

enum class ContractStatus { Active, Completed, Failed, Cancelled };

// A live contract instance accepted by the player.
struct ContractInstance {
  std::string uid;
  std::string template_id;
  std::string type;
  std::string title;
  std::string description;
  Client client;
  double reward = 0.0;
  double penalty = 0.0;
  std::uint64_t deadline_minute = 0;   // absolute game minute
  std::uint64_t accepted_minute = 0;
  ContractStatus status = ContractStatus::Active;
  double progress = 0.0;               // 0..1 (courier route progress / SLA accrual)
  json requirements;
  double risk = 0.0;
  json rep_effects;
  // courier minigame state
  int route_index = -1;                // chosen route
  bool courier_started = false;
  double courier_time_h = 0.0;
  double courier_risk = 0.0;
  // passive income tracking (storage/hosting)
  double income_accrued = 0.0;
  double sla_ok = 0.0;
  double sla_total = 0.0;

  bool expired(std::uint64_t now) const { return status == ContractStatus::Active && now > deadline_minute; }

  json to_json() const;
  static ContractInstance from_json(const json& j);
};

} // namespace neon::economy
