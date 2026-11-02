#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "netrunning/Cyberdeck.hpp"
#include "netrunning/GridMap.hpp"
#include "netrunning/Program.hpp"

namespace neon::core {
class Game;
}

namespace neon::netrunning {

using nlohmann::json;

// A live netrun (turn-based grid intrusion). Not persisted; netruns are transient.
class NetrunSession {
public:
  enum class State { Running, Won, Lost, Traced, Fled };

  NetrunSession() = default;
  // Begin a run against `target`. `use_proxy` routes through a claimed server
  // (longer trace timer, less heat).
  static NetrunSession start(neon::core::Game& game, const NetrunTarget& target, bool use_proxy);

  GridMap map;
  NetrunTarget target;
  int px = 0;
  int py = 0;
  int shield = 100;
  int max_shield = 100;
  int action_points = 2;
  double detection = 0.0; // 0..100
  double trace = 0.0;     // 0..100, fills over time
  int turn = 0;
  State state = State::Running;
  bool used_proxy = false;
  std::vector<std::string> programs;        // equipped program ids
  std::map<std::string, int> cooldowns;     // program id -> turns remaining
  int ram_used = 0;
  int shards_stolen = 0;
  int credits_stolen = 0;
  std::vector<std::string> log;

  void add_log(const std::string& line);

  bool is_running() const { return state == State::Running; }

  // Player actions. Each costs 1 action point. Return false if not allowed.
  bool move(int dx, int dy);
  bool use_program(const std::string& program_id);
  void flee(); // step onto Exit node and jack out, keeping loot

  // End the player turn: ICE acts, trace advances, AP resets.
  void end_turn();

  // Helpers used by UI/tests.
  int actions_per_turn() const;
  double trace_rate() const; // percent per turn
  bool ice_at(int x, int y) const;
  ICE* ice_at(int x, int y);
  int adjacent_ice_count() const;
  const ProgramDef* program_def(const std::string& id) const;
  int program_cooldown(const std::string& id) const;

  json to_json() const;

private:
  neon::core::Game* game_ = nullptr;
  void ice_phase();
  void detection_check();
  bool damage_adjacent_ice(double amount);
  bool loot_adjacent_data(bool spike);
};

} // namespace neon::netrunning
