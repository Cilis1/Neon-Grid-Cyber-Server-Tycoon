#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/DataCatalog.hpp"
#include "core/EventBus.hpp"
#include "core/GameState.hpp"
#include "core/LuaEngine.hpp"
#include "netrunning/NetrunSession.hpp"
#include "techtree/TechTree.hpp"
#include "utils/Random.hpp"

namespace neon::core {

// Central orchestrator: owns state, catalog, rng, systems. UI and tests drive
// the game exclusively through this class.
class Game {
public:
  Game();

  // Load data files. `data_dir_override` empty -> auto-detect next to exe/CWD.
  bool init(const std::optional<std::filesystem::path>& data_dir_override = std::nullopt,
            const std::optional<std::filesystem::path>& scripts_dir_override = std::nullopt);

  void new_game(std::uint64_t seed = 0);
  bool save(const std::filesystem::path& path) const;
  bool load(const std::filesystem::path& path);
  void load_state(GameState new_state);

  // Advance the simulation by one in-game minute (or n minutes).
  void tick();
  void tick_minutes(std::uint64_t minutes);

  // Accessors
  GameState& state() { return state_; }
  const GameState& state() const { return state_; }
  DataCatalog& catalog() { return catalog_; }
  const DataCatalog& catalog() const { return catalog_; }
  utils::Rng& rng() { return rng_; }
  EventBus& bus() { return bus_; }
  techtree::TechTree& tech() { return tech_; }
  LuaEngine& lua() { return lua_; }
  netrunning::NetrunSession* session() { return session_ ? &*session_ : nullptr; }
  const netrunning::NetrunSession* session() const { return session_ ? &*session_ : nullptr; }

  // Effective cyberdeck stats (base + tech + cyberware).
  netrunning::Cyberdeck deck_stats() const;
  // Aggregated stat bonus (additive) and multiplier from tech tree.
  double get_bonus(const std::string& name) const;
  double get_multiplier(const std::string& name) const;
  double get_stat(const std::string& name) const { return get_bonus(name); }

  bool is_contract_type_unlocked(const std::string& type) const;
  bool is_item_unlocked(const std::string& item_id) const;
  void add_heat(int amount);
  void bribe(); // pay credits to reduce heat

  hardware::Room* room(const std::string& id);
  hardware::Room& active_room();
  double room_uptime(const std::string& room_id) const;
  double room_total_storage_tb(const std::string& room_id) const;
  double room_total_mbps(const std::string& room_id) const;
  double room_total_hash() const;
  hardware::RoomStats room_stats(const std::string& room_id);

  // Unique id generator.
  std::string uid(const std::string& prefix);

  // ---- shop / inventory / room ----
  bool buy_hardware(const std::string& def_id);            // -> inventory
  bool sell_inventory_item(std::size_t inventory_index);
  bool install_in_rack(const std::string& component_uid, const std::string& rack_uid);
  bool install_part(const std::string& part_uid, const std::string& server_uid, const std::string& slot_type);
  bool place_on_tile(const std::string& component_uid, int x, int y);
  bool move_to_inventory(const std::string& component_uid);
  hardware::Cable* connect_cable(const std::string& from_uid, const std::string& from_port,
                                 const std::string& to_uid, const std::string& to_port);
  bool clean_component(const std::string& uid);            // uses canned air
  bool repair_component(const std::string& uid);
  bool reset_breaker(const std::string& room_id);
  bool expand_room(const std::string& room_id);            // buy adjacent room expansion

  // ---- research ----
  bool research(const std::string& node_id);

  // ---- netrun ----
  bool start_netrun(const std::string& target_id, bool use_proxy);
  // Apply the outcome of the current session (called when session ends).
  void resolve_netrun();

  // ---- employees ----
  std::vector<Employee> generate_hires();
  Employee* hire(const Employee& candidate);
  bool fire(const std::string& uid);
  int max_employees() const;

  // ---- events ----
  void roll_random_event();
  void apply_event(const EventDef& def);

  // ---- economy helpers ----
  double power_bill_per_day() const;
  double rent_per_day() const;
  double mining_credits_per_hour() const;
  void sell_crypto(double fraction = 1.0);

  // settings
  bool scanlines = true;
  bool glitch_text = true;
  SimSpeed speed = SimSpeed::Normal;
  float volume = 0.8f;
  bool fullscreen = false;

private:
  DataCatalog catalog_;
  GameState state_;
  utils::Rng rng_;
  EventBus bus_;
  techtree::TechTree tech_;
  LuaEngine lua_;
  std::optional<netrunning::NetrunSession> session_;
  std::uint64_t uid_counter_ = 0;
  int heat_decay_acc_ = 0;
  bool raid_done_ = false;

  void init_defaults();
  void daily_rollover();
  void weekly_rollover();
  void hourly_rollover();
  void check_heat_thresholds();
  void employee_tasks(std::uint64_t minutes);
  void generate_netrun_targets();
};

} // namespace neon::core
