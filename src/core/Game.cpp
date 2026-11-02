#include "core/Game.hpp"

#include "hardware/HardwareFactory.hpp"
#include "hardware/Rack.hpp"
#include "hardware/Server.hpp"
#include "techtree/Effect.hpp"
#include "utils/FileSystem.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace neon::core {

Game::Game() {
  rng_ = utils::Rng(0xC0FFEE);
}

bool Game::init(const std::optional<std::filesystem::path>& data_dir_override,
                const std::optional<std::filesystem::path>& scripts_dir_override) {
  std::string err;
  auto data_dir = utils::fs::find_data_dir(data_dir_override);
  if (!data_dir) {
    spdlog::error("could not locate data/ directory");
    return false;
  }
  if (!catalog_.load_from_dir(*data_dir, &err)) {
    spdlog::error("data catalog load failed: {}", err);
    return false;
  }
  tech_.load(catalog_.tech_nodes());

  std::filesystem::path scripts_dir;
  if (scripts_dir_override) scripts_dir = *scripts_dir_override;
  else {
    scripts_dir = data_dir->parent_path() / "scripts";
    if (!std::filesystem::exists(scripts_dir)) scripts_dir = std::filesystem::current_path() / "scripts";
  }
  lua_.init(scripts_dir);
  return true;
}

std::string Game::uid(const std::string& prefix) {
  return prefix + "_" + std::to_string(++uid_counter_) + "_" + std::to_string(rng_.next_u64() % 100000);
}

void Game::new_game(std::uint64_t seed) {
  state_ = GameState{};
  state_.seed = seed ? seed : 0xBADC0DE;
  rng_ = utils::Rng(state_.seed);
  state_.rng_state = rng_.state();
  uid_counter_ = 0;

  // starter room
  state_.rooms.clear();
  state_.rooms["room1"] = hardware::create_starter_room(catalog_, [this](const std::string& p) { return uid(p); });
  state_.active_room = "room1";

  // starting inventory: a spare CPU + canned air
  hardware::Component spare_cpu = hardware::make_component("hw_cpu_t2", uid("cpu"));
  state_.inventory.push_back(spare_cpu);
  state_.inventory_count["consumable_canned_air"] = 2;
  state_.inventory_count["consumable_thermal_paste"] = 1;

  // base unlocks: tier-1 hardware is always in the shop
  for (const auto& [id, def] : catalog_.hardware_items())
    if (def.tier <= 1) state_.unlocked_items.insert(id);

  // faction base rep
  for (const auto& [id, f] : catalog_.factions()) state_.faction_rep[id] = f.base_rep;

  // market initial prices
  for (const auto& [id, d] : catalog_.market_items())
    state_.market_prices[id] = (d.min_price + d.max_price) / 2.0;

  generate_netrun_targets();
  state_.contracts.refresh_offers(*this);

  // starting cyberdeck
  state_.cyberdeck = netrunning::Cyberdeck{8, 1.0, 100, 100, 4};

  raid_done_ = false;
  heat_decay_acc_ = 0;
  bus_.publish("game.new", {{"seed", state_.seed}});
  spdlog::info("new game started (seed {})", state_.seed);
}

void Game::load_state(GameState new_state) {
  state_ = std::move(new_state);
  rng_.set_state(state_.rng_state);
  uid_counter_ = 1000000; // avoid uid collisions with loaded content
  tech_.load(catalog_.tech_nodes());
  raid_done_ = false;
}

bool Game::save(const std::filesystem::path& path) const {
  // serialize the state, syncing the rng state alongside
  json j = state_.to_json();
  j["rng_state"] = rng_.state();
  return utils::fs::write_text_file(path, j.dump(2));
}

bool Game::load(const std::filesystem::path& path) {
  auto text = utils::fs::read_text_file(path);
  if (!text) return false;
  try {
    json j = json::parse(*text);
    GameState s = GameState::from_json(j);
    if (s.version > SAVE_VERSION) {
      spdlog::error("save version {} newer than supported {}", s.version, SAVE_VERSION);
      return false;
    }
    // migrations
    if (s.version < 2) {
      if (s.unlocked_contract_types.empty()) s.unlocked_contract_types = {"courier", "storage", "hosting"};
    }
    if (s.version < 3) {
      for (const auto& [id, def] : catalog_.hardware_items())
        if (def.tier <= 1) s.unlocked_items.insert(id);
    }
    s.version = SAVE_VERSION;
    load_state(std::move(s));
    if (j.contains("rng_state")) rng_.set_state(j["rng_state"].get<std::uint64_t>());
    return true;
  } catch (const std::exception& e) {
    spdlog::error("failed to load save {}: {}", path.string(), e.what());
    return false;
  }
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

double Game::get_bonus(const std::string& name) const {
  auto it = state_.stat_bonuses.find(name);
  double base = it == state_.stat_bonuses.end() ? 0.0 : it->second;
  // cyberware contributions
  if (name == "max_ram_bonus" && state_.cyberware.count("neural_link") && state_.cyberware.at("neural_link")) base += 4.0;
  if (name == "action_points" && state_.cyberware.count("reflex_tuner") && state_.cyberware.at("reflex_tuner")) base += 1.0;
  if (name == "ice_damage_bonus" && state_.cyberware.count("ice_breaker") && state_.cyberware.at("ice_breaker")) base += 8.0;
  if (name == "trace_reduction" && state_.cyberware.count("ghost_protocol") && state_.cyberware.at("ghost_protocol")) base += 1.5;
  if (name == "shield_bonus" && state_.cyberware.count("dermal_plating") && state_.cyberware.at("dermal_plating")) base += 25.0;
  return base;
}

double Game::get_multiplier(const std::string& name) const {
  auto it = state_.stat_multipliers.find(name);
  return it == state_.stat_multipliers.end() ? 1.0 : it->second;
}

netrunning::Cyberdeck Game::deck_stats() const {
  netrunning::Cyberdeck d = state_.cyberdeck;
  d.ram += static_cast<int>(get_bonus("max_ram_bonus"));
  d.program_slots += static_cast<int>(get_bonus("program_slots"));
  d.max_shield += static_cast<int>(get_bonus("shield_bonus"));
  d.shield = d.max_shield;
  d.cpu_speed = d.cpu_speed * get_multiplier("cpu_speed");
  return d;
}

bool Game::is_contract_type_unlocked(const std::string& type) const {
  return state_.unlocked_contract_types.count(type) > 0;
}

bool Game::is_item_unlocked(const std::string& item_id) const {
  const hardware::HardwareItemDef* def = catalog_.hardware(item_id);
  if (!def) return false;
  if (def->tier <= 1) return true;
  return state_.unlocked_items.count(item_id) > 0;
}

void Game::add_heat(int amount) {
  int before = state_.heat;
  state_.heat = std::clamp(state_.heat + amount, 0, 100);
  if (state_.heat != before) bus_.publish("heat.changed", {{"heat", state_.heat}, {"delta", amount}});
}

void Game::bribe() {
  const double cost = 1000.0 + state_.heat * 20.0;
  if (state_.credits >= cost) {
    state_.credits -= cost;
    add_heat(-30);
    spdlog::info("bribe paid: {:.0f} credits, heat reduced", cost);
    bus_.publish("heat.bribed", {{"cost", cost}});
  }
}

// ---------------------------------------------------------------------------
// Rooms / hardware
// ---------------------------------------------------------------------------

hardware::Room* Game::room(const std::string& id) {
  auto it = state_.rooms.find(id);
  return it == state_.rooms.end() ? nullptr : &it->second;
}

hardware::Room& Game::active_room() { return state_.rooms.at(state_.active_room); }

double Game::room_uptime(const std::string& room_id) const {
  auto it = state_.rooms.find(room_id);
  if (it == state_.rooms.end()) return 1.0;
  const auto& r = it->second;
  if (r.sla_total_minutes <= 0) return 1.0;
  return r.sla_ok_minutes / r.sla_total_minutes;
}

double Game::room_total_storage_tb(const std::string& room_id) const {
  auto it = state_.rooms.find(room_id);
  if (it == state_.rooms.end()) return 0.0;
  double total = 0.0;
  for (const auto& c : it->second.components) {
    const hardware::HardwareItemDef* def = catalog_.hardware(c.def_id);
    if (def && def->type == "server_unit" && c.online && c.powered) {
      total += hardware::compute_server_stats(catalog_, it->second, c).storage_tb;
    }
  }
  return total;
}

double Game::room_total_mbps(const std::string& room_id) const {
  auto it = state_.rooms.find(room_id);
  if (it == state_.rooms.end()) return 0.0;
  double total = 0.0;
  for (const auto& c : it->second.components) {
    const hardware::HardwareItemDef* def = catalog_.hardware(c.def_id);
    if (def && def->type == "switch" && c.online && c.powered) total += def->stat("mbps", 100);
  }
  return total;
}

double Game::room_total_hash() const {
  double total = 0.0;
  for (const auto& [id, r] : state_.rooms)
    for (const auto& c : r.components) {
      const hardware::HardwareItemDef* def = catalog_.hardware(c.def_id);
      if (def && def->type == "server_unit" && c.online && c.powered)
        total += hardware::compute_server_stats(catalog_, r, c).hash_mh;
    }
  return total * get_multiplier("mining_bonus") * (1.0 + get_bonus("mining_bonus"));
}

hardware::RoomStats Game::room_stats(const std::string& room_id) {
  auto* r = room(room_id);
  if (!r) return {};
  return hardware::recompute_room_stats(catalog_, *r, 1.0 + get_bonus("cooling_efficiency"));
}

bool Game::buy_hardware(const std::string& def_id) {
  const hardware::HardwareItemDef* def = catalog_.hardware(def_id);
  if (!def || def->type == "consumable") return false;
  if (!is_item_unlocked(def_id)) return false;
  if (state_.credits < def->price) return false;
  state_.credits -= def->price;
  state_.inventory.push_back(hardware::make_component(def_id, uid(def->type)));
  spdlog::info("bought {} for {:.0f}", def->name, def->price);
  bus_.publish("shop.bought", {{"id", def_id}});
  return true;
}

bool Game::sell_inventory_item(std::size_t inventory_index) {
  if (inventory_index >= state_.inventory.size()) return false;
  const auto& c = state_.inventory[inventory_index];
  const hardware::HardwareItemDef* def = catalog_.hardware(c.def_id);
  if (!def) return false;
  double price = def->price * 0.6;
  state_.credits += price;
  state_.inventory.erase(state_.inventory.begin() + static_cast<std::ptrdiff_t>(inventory_index));
  return true;
}

bool Game::install_in_rack(const std::string& component_uid, const std::string& rack_uid) {
  hardware::Room& r = active_room();
  hardware::Component* comp = nullptr;
  for (auto& c : state_.inventory)
    if (c.uid == component_uid) comp = &c;
  if (!comp) {
    comp = r.find(component_uid);
    if (!comp) return false;
  }
  hardware::Component* rack = r.find(rack_uid);
  if (!rack) return false;
  const hardware::HardwareItemDef* cdef = catalog_.hardware(comp->def_id);
  const hardware::HardwareItemDef* rdef = catalog_.hardware(rack->def_id);
  if (!cdef || !rdef || rdef->type != "rack" || cdef->type != "server_unit") return false;
  if (hardware::rack_used_u(catalog_, r, *rack) + cdef->size_u > hardware::rack_capacity_u(catalog_, *rack)) return false;
  // move component from inventory into the room
  if (comp->tile_index == -1 && r.find(component_uid) == nullptr) {
    state_.inventory.erase(std::remove_if(state_.inventory.begin(), state_.inventory.end(),
                                          [&](const hardware::Component& c) { return c.uid == component_uid; }),
                           state_.inventory.end());
    r.components.push_back(*comp);
    comp = r.find(component_uid);
  }
  return hardware::rack_add_server(r, *rack, component_uid);
}

bool Game::install_part(const std::string& part_uid, const std::string& server_uid, const std::string& slot_type) {
  hardware::Room& r = active_room();
  hardware::Component* server = r.find(server_uid);
  if (!server) return false;
  const hardware::HardwareItemDef* sdef = catalog_.hardware(server->def_id);
  if (!sdef || sdef->type != "server_unit") return false;

  hardware::Component* part = nullptr;
  for (auto& c : state_.inventory)
    if (c.uid == part_uid) part = &c;
  if (!part) {
    part = r.find(part_uid);
    if (!part) return false;
  }
  const hardware::HardwareItemDef* pdef = catalog_.hardware(part->def_id);
  if (!pdef || pdef->slot_type != slot_type) return false;
  // compatibility check (empty list = universal)
  if (!pdef->compatibility.empty()) {
    bool ok = false;
    for (const auto& c : pdef->compatibility)
      if (c == sdef->type || c == sdef->id) ok = true;
    if (!ok) return false;
  }
  if (part->tile_index == -1 && r.find(part_uid) == nullptr) {
    state_.inventory.erase(std::remove_if(state_.inventory.begin(), state_.inventory.end(),
                                          [&](const hardware::Component& c) { return c.uid == part_uid; }),
                           state_.inventory.end());
    r.components.push_back(*part);
    part = r.find(part_uid);
    if (!part) return false;
  }
  server->slots[slot_type] = part_uid;
  part->parent_uid = server_uid;
  return true;
}

bool Game::place_on_tile(const std::string& component_uid, int x, int y) {
  hardware::Room& r = active_room();
  hardware::Component* comp = nullptr;
  for (auto& c : state_.inventory)
    if (c.uid == component_uid) comp = &c;
  if (!comp) return false;
  const hardware::HardwareItemDef* def = catalog_.hardware(comp->def_id);
  if (!def) return false;
  hardware::TileKind kind = hardware::TileKind::Empty;
  if (def->type == "rack") kind = hardware::TileKind::Rack;
  else if (def->type == "cooler") kind = hardware::TileKind::Cooler;
  else if (def->type == "pdu") kind = hardware::TileKind::PDU;
  else if (def->type == "ups") kind = hardware::TileKind::UPS;
  else if (def->type == "switch") kind = hardware::TileKind::CableTrunk;
  else return false; // server units / parts go into racks, not on tiles
  if (!r.place_on_tile(component_uid, x, y)) return false;
  // move from inventory into room
  state_.inventory.erase(std::remove_if(state_.inventory.begin(), state_.inventory.end(),
                                        [&](const hardware::Component& c) { return c.uid == component_uid; }),
                         state_.inventory.end());
  r.components.push_back(*comp);
  if (hardware::Component* placed = r.find(component_uid)) {
    if (hardware::Tile* t = r.tile_at(x, y)) {
      t->kind = kind;
      t->occupant_uid = component_uid;
    }
    placed->tile_index = r.tile_index(x, y);
  }
  return true;
}

bool Game::move_to_inventory(const std::string& component_uid) {
  hardware::Room& r = active_room();
  hardware::Component* c = r.find(component_uid);
  if (!c) return false;
  const hardware::HardwareItemDef* def = catalog_.hardware(c->def_id);
  if (!def) return false;
  if (def->type == "server_unit") {
    // must be removed from rack first
    if (!c->parent_uid.empty()) return false;
  }
  if (def->type == "cpu" || def->type == "ram" || def->type == "storage" || def->type == "psu" ||
      def->type == "fan" || def->type == "gpu" || def->type == "network_card" || def->type == "cyberdeck") {
    // must be removed from server slot first
    if (!c->parent_uid.empty()) return false;
  }
  r.remove_from_tile(component_uid);
  // detach from parent rack
  if (!c->parent_uid.empty()) {
    if (hardware::Component* rack = r.find(c->parent_uid)) hardware::rack_remove_server(r, *rack, component_uid);
    c->parent_uid.clear();
  }
  state_.inventory.push_back(*c);
  r.components.erase(std::remove_if(r.components.begin(), r.components.end(),
                                     [&](const hardware::Component& x) { return x.uid == component_uid; }),
                      r.components.end());
  return true;
}

hardware::Cable* Game::connect_cable(const std::string& from_uid, const std::string& from_port,
                                     const std::string& to_uid, const std::string& to_port) {
  hardware::Room& r = active_room();
  if (!r.find(from_uid) || !r.find(to_uid)) return nullptr;
  return &hardware::add_cable(r, [this](const std::string& p) { return uid(p); }, from_uid, from_port, to_uid, to_port);
}

bool Game::clean_component(const std::string& uid_str) {
  if (state_.inventory_count["consumable_canned_air"] <= 0) return false;
  for (auto& [id, r] : state_.rooms) {
    if (hardware::Component* c = r.find(uid_str)) {
      c->dust_level = 0;
      c->dust_frac = 0.0;
      state_.inventory_count["consumable_canned_air"]--;
      spdlog::info("cleaned dust off {}", c->def_id);
      return true;
    }
  }
  return false;
}

bool Game::repair_component(const std::string& uid_str) {
  const double cost = 150.0;
  if (state_.credits < cost) return false;
  for (auto& [id, r] : state_.rooms) {
    if (hardware::Component* c = r.find(uid_str)) {
      state_.credits -= cost;
      c->durability = 100;
      c->wear_level = std::max(0, c->wear_level - 50);
      c->online = true;
      return true;
    }
  }
  return false;
}

bool Game::reset_breaker(const std::string& room_id) {
  hardware::Room* r = room(room_id);
  if (!r || !r->breaker_tripped) return false;
  r->breaker_tripped = false;
  spdlog::info("breaker reset in {}", room_id);
  return true;
}

bool Game::expand_room(const std::string& room_id) {
  hardware::Room* r = room(room_id);
  if (!r) return false;
  double cost = 5000.0 * (1 + state_.rooms.size());
  if (state_.credits < cost) return false;
  state_.credits -= cost;
  std::string new_id = "room" + std::to_string(state_.rooms.size() + 1);
  hardware::Room expansion;
  expansion.id = new_id;
  expansion.name = "Expansion " + std::to_string(state_.rooms.size());
  expansion.width = 12;
  expansion.height = 10;
  expansion.init_grid();
  // each expansion comes with a PDU so it is usable immediately
  hardware::Component pdu = hardware::make_component("hw_pdu_t1", uid("pdu"));
  expansion.components.push_back(pdu);
  expansion.components.back().tile_index = expansion.tile_index(1, 1);
  if (hardware::Tile* t = expansion.tile_at(1, 1)) {
    t->kind = hardware::TileKind::PDU;
    t->occupant_uid = expansion.components.back().uid;
  }
  state_.rooms[new_id] = expansion;
  spdlog::info("bought expansion room {}", new_id);
  return true;
}

// ---------------------------------------------------------------------------
// Research
// ---------------------------------------------------------------------------

bool Game::research(const std::string& node_id) { return tech_.purchase(*this, node_id); }

// ---------------------------------------------------------------------------
// Netrunning
// ---------------------------------------------------------------------------

bool Game::start_netrun(const std::string& target_id, bool use_proxy) {
  for (auto& t : state_.netrun_targets) {
    if (t.id == target_id && !t.claimed) {
      if (use_proxy && state_.remote_servers.empty()) return false; // need a proxy
      session_ = netrunning::NetrunSession::start(*this, t, use_proxy && !state_.remote_servers.empty());
      // illegal runs generate heat
      if (t.kind != "abandoned") add_heat(5 + t.difficulty * 3);
      else add_heat(2);
      bus_.publish("netrun.started", {{"target", target_id}});
      return true;
    }
  }
  return false;
}

void Game::resolve_netrun() {
  if (!session_) return;
  auto& s = *session_;
  auto it = std::find_if(state_.netrun_targets.begin(), state_.netrun_targets.end(),
                         [&](const netrunning::NetrunTarget& t) { return t.id == s.target.id; });
  switch (s.state) {
    case netrunning::NetrunSession::State::Won: {
      state_.credits += s.credits_stolen + s.target.reward_credits;
      state_.data_shards += s.shards_stolen + s.target.reward_shards;
      state_.netruns_won++;
      if (it != state_.netrun_targets.end()) {
        it->claimed = true;
        // claim abandoned servers as remote infrastructure
        if (it->kind == "abandoned") {
          RemoteServer rs;
          rs.id = it->id;
          rs.name = it->name;
          rs.kind = it->kind;
          rs.income_per_hour = 15.0 + it->difficulty * 10.0;
          rs.heat_risk = 0.5;
          rs.proxy = true;
          state_.remote_servers.push_back(rs);
          spdlog::info("claimed remote server {} (+{:.0f} cr/h)", rs.name, rs.income_per_hour);
        }
      }
      add_heat(static_cast<int>(s.target.heat_risk * 50) + (s.used_proxy ? 0 : 10));
      bus_.publish("netrun.won", {{"target", s.target.id}, {"shards", s.shards_stolen}, {"credits", s.credits_stolen}});
      break;
    }
    case netrunning::NetrunSession::State::Fled: {
      state_.credits += s.credits_stolen;
      state_.data_shards += s.shards_stolen;
      bus_.publish("netrun.fled", {{"target", s.target.id}});
      break;
    }
    case netrunning::NetrunSession::State::Traced: {
      add_heat(30 + s.target.difficulty * 5);
      state_.credits = std::max(0.0, state_.credits - 500.0);
      state_.reputation -= 5;
      bus_.publish("netrun.traced", {{"target", s.target.id}});
      break;
    }
    case netrunning::NetrunSession::State::Lost: {
      add_heat(15 + s.target.difficulty * 5);
      state_.credits = std::max(0.0, state_.credits - 250.0);
      bus_.publish("netrun.lost", {{"target", s.target.id}});
      break;
    }
    default: break;
  }
  session_.reset();
}

// ---------------------------------------------------------------------------
// Employees
// ---------------------------------------------------------------------------

int Game::max_employees() const { return 2 + static_cast<int>(get_bonus("employee_slots")); }

std::vector<Employee> Game::generate_hires() {
  static const char* names[] = {"Riko", "Vex", "Moss", "Juno", "Kade", "Sable", "Ilya", "Nix", "Petra", "Onyx"};
  static const char* roles[] = {"maintenance", "netrunner", "manager", "security"};
  std::vector<Employee> out;
  for (int i = 0; i < 3; ++i) {
    Employee e;
    e.uid = uid("emp");
    e.name = names[rng_.next_int(0, 9)];
    e.role = roles[rng_.next_int(0, 3)];
    e.skill = rng_.next_int(1, 5);
    e.salary_weekly = 200.0 * e.skill * (e.role == "security" ? 1.3 : 1.0);
    e.morale = rng_.next_int(60, 95);
    out.push_back(e);
  }
  return out;
}

Employee* Game::hire(const Employee& candidate) {
  if (static_cast<int>(state_.employees.size()) >= max_employees()) return nullptr;
  state_.employees.push_back(candidate);
  bus_.publish("employee.hired", {{"name", candidate.name}, {"role", candidate.role}});
  return &state_.employees.back();
}

bool Game::fire(const std::string& uid_str) {
  auto it = std::find_if(state_.employees.begin(), state_.employees.end(),
                         [&](const Employee& e) { return e.uid == uid_str; });
  if (it == state_.employees.end()) return false;
  state_.employees.erase(it);
  return true;
}

void Game::employee_tasks(std::uint64_t minutes) {
  (void)minutes; // tasks run per hourly batch
  for (auto& e : state_.employees) {
    if (e.role == "maintenance") {
      // auto-clean the dustiest component if canned air available
      if (state_.inventory_count["consumable_canned_air"] > 0) {
        hardware::Component* dustiest = nullptr;
        int best = -1;
        for (auto& [id, r] : state_.rooms)
          for (auto& c : r.components)
            if (c.dust_level > best) {
              best = c.dust_level;
              dustiest = &c;
            }
        if (dustiest && best >= 50 + e.skill * 5) {
          dustiest->dust_level = std::max(0, dustiest->dust_level - 40 - e.skill * 10);
          state_.inventory_count["consumable_canned_air"]--;
        }
      }
    } else if (e.role == "security") {
      add_heat(-static_cast<int>(e.skill)); // per tick batch (called hourly)
    } else if (e.role == "netrunner") {
      if (rng_.chance(0.3 + e.skill * 0.1)) {
        double shards = e.skill;
        state_.data_shards += shards;
        bus_.publish("employee.netrun", {{"name", e.name}, {"shards", shards}});
      }
    } else if (e.role == "manager") {
      // small chance to auto-accept a courier contract
      if (rng_.chance(0.2) && !state_.contracts.offers.empty()) {
        auto& offer = state_.contracts.offers.front();
        if (offer.type == "courier" && state_.contracts.can_accept(*this, offer.uid))
          state_.contracts.accept(*this, offer.uid);
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Random events
// ---------------------------------------------------------------------------

void Game::apply_event(const EventDef& def) {
  spdlog::info("event: {} - {}", def.name, def.description);
  bus_.publish("event.fired", {{"id", def.id}, {"name", def.name}});
  // Lua first if a script is present
  if (!def.lua_script.empty()) {
    json ctx = {{"event_id", def.id}, {"credits", state_.credits}, {"heat", state_.heat}};
    json result = lua_.run_event_script(def.lua_script, ctx);
    if (result.is_object() && result.contains("effects") && result["effects"].is_array()) {
      for (auto& e : result["effects"]) techtree::apply_effect(*this, techtree::Effect::from_json(e));
      return;
    } else if (!result.is_null() && result.is_object() && !result.empty()) {
      // script returned a flat effect
      techtree::apply_effect(*this, techtree::Effect::from_json(result));
      return;
    }
    // fall through to C++ effects if lua produced nothing
  }
  for (const auto& e : def.effects) techtree::apply_effect(*this, e);
}

void Game::roll_random_event() {
  std::vector<const EventDef*> eligible;
  double total_weight = 0.0;
  for (const auto& [id, def] : catalog_.events()) {
    bool ok = true;
    if (def.conditions.contains("heat_min") && state_.heat < def.conditions["heat_min"].get<int>()) ok = false;
    if (def.conditions.contains("heat_max") && state_.heat > def.conditions["heat_max"].get<int>()) ok = false;
    if (def.conditions.contains("time_of_day") && def.conditions["time_of_day"] == "night" && !state_.time.is_night()) ok = false;
    if (def.conditions.contains("time_of_day") && def.conditions["time_of_day"] == "day" && state_.time.is_night()) ok = false;
    if (def.conditions.contains("min_rooms") && static_cast<int>(state_.rooms.size()) < def.conditions["min_rooms"].get<int>()) ok = false;
    if (ok) {
      eligible.push_back(&def);
      total_weight += def.weight;
    }
  }
  if (eligible.empty() || total_weight <= 0) return;
  double roll = rng_.next_double() * total_weight;
  for (auto* def : eligible) {
    roll -= def->weight;
    if (roll <= 0) {
      apply_event(*def);
      return;
    }
  }
}

// ---------------------------------------------------------------------------
// Economy helpers
// ---------------------------------------------------------------------------

double Game::power_bill_per_day() const {
  double watts = 0.0;
  for (const auto& [id, r] : state_.rooms) {
    hardware::Room copy = r; // recompute on a copy (const method)
    auto stats = hardware::recompute_room_stats(catalog_, copy, 1.0 + get_bonus("cooling_efficiency"));
    watts += stats.power_draw_w;
  }
  double kwh = watts * 24.0 / 1000.0;
  double eff = 1.0 + get_bonus("power_efficiency");
  return kwh * 0.15 / std::max(0.5, eff);
}

double Game::rent_per_day() const { return 100.0 * state_.rooms.size(); }

double Game::mining_credits_per_hour() const {
  if (state_.crypto_amount < 0) return 0.0;
  double hash = room_total_hash();
  if (hash <= 0) return 0.0;
  double price = 0.0;
  auto it = state_.market_prices.find("crypto_neon");
  if (it != state_.market_prices.end()) price = it->second;
  else price = 2.0;
  return hash * price * 0.01; // hash_mh * price conversion factor
}

void Game::sell_crypto(double fraction) {
  if (state_.crypto_amount <= 0) return;
  double amount = state_.crypto_amount * fraction;
  double price = state_.market_prices.count("crypto_neon") ? state_.market_prices["crypto_neon"] : 2.0;
  state_.credits += amount * price;
  state_.crypto_amount -= amount;
}

// ---------------------------------------------------------------------------
// Netrun target generation
// ---------------------------------------------------------------------------

void Game::generate_netrun_targets() {
  state_.netrun_targets.clear();
  struct Spec {
    std::string name;
    std::string kind;
    std::string faction;
    int diff;
  };
  std::vector<Spec> specs = {
      {"Old Laundry Server", "abandoned", "", 1},
      {"Vending Machine Cluster", "abandoned", "", 1},
      {"Forgotten Arcade Cabinet", "abandoned", "", 2},
      {"Motel Payphone Node", "abandoned", "", 2},
      {"Abandoned Drone Depot", "abandoned", "", 3},
      {"Arasoft Regional Cache", "corporate", "arasaka", 3},
      {"Kurosawa Logistics Hub", "corporate", "arasaka", 4},
      {"Helix Biotech Vault", "corporate", "arasaka", 5},
      {"Neon Serpents Dead Drop", "faction", "neon_serpents", 3},
      {"Glitch Collective Relay", "faction", "glitch_collective", 4},
      {"CityNet Security Node", "faction", "citynet", 4},
      {"The Loa Shrine", "faction", "loa", 5},
  };
  int i = 0;
  for (const auto& s : specs) {
    netrunning::NetrunTarget t;
    t.id = "target_" + std::to_string(++i);
    t.name = s.name;
    t.kind = s.kind;
    t.faction_id = s.faction;
    t.difficulty = s.diff;
    t.reward_credits = 300 * s.diff * (s.kind == "corporate" ? 3 : 1);
    t.reward_shards = s.diff * (s.kind == "faction" ? 3 : 1);
    t.heat_risk = s.kind == "corporate" ? 0.8 : (s.kind == "faction" ? 0.5 : 0.2);
    t.passive_income = s.kind == "abandoned" ? 10.0 * s.diff : 0.0;
    state_.netrun_targets.push_back(t);
  }
}

// ---------------------------------------------------------------------------
// Simulation tick
// ---------------------------------------------------------------------------

void Game::tick() { tick_minutes(1); }

void Game::tick_minutes(std::uint64_t minutes) {
  for (std::uint64_t m = 0; m < minutes; ++m) {
    state_.time.advance(1);
    rng_.set_state(rng_.state()); // state already advances internally; keep in sync
    state_.rng_state = rng_.state();

    // rooms physics
    for (auto& [id, r] : state_.rooms) hardware::simulate_room(*this, r, 1);

    // contracts
    state_.contracts.tick(*this, 1);

    // heat decay: -1 per in-game hour
    heat_decay_acc_++;
    if (heat_decay_acc_ >= 60) {
      heat_decay_acc_ = 0;
      if (state_.heat > 0) add_heat(-1);
    }

    // random event roll every 2 in-game hours
    if (state_.time.minute - state_.last_event_roll_minute >= 120) {
      state_.last_event_roll_minute = state_.time.minute;
      if (rng_.chance(0.5)) roll_random_event();
    }

    check_heat_thresholds();

    // hourly systems
    if (state_.time.minute % 60 == 0) hourly_rollover();
    // daily systems
    if (state_.time.minute % TimeSystem::MINUTES_PER_DAY == 0) daily_rollover();
  }
  state_.rng_state = rng_.state();
}

void Game::hourly_rollover() {
  // mining income
  double mining = mining_credits_per_hour();
  if (mining > 0) {
    state_.crypto_amount += room_total_hash() * 0.001;
    state_.credits += mining;
    state_.total_earned += static_cast<int>(mining);
  }
  // remote server income
  for (const auto& rs : state_.remote_servers) {
    double income = rs.income_per_hour * (1.0 + get_bonus("income_bonus") * 0.01);
    state_.credits += income;
    state_.total_earned += static_cast<int>(income);
  }
  employee_tasks(60);
}

void Game::daily_rollover() {
  std::uint64_t day = state_.time.day();
  // power bill + rent + internet
  double bill = power_bill_per_day() + rent_per_day() + 20.0;
  state_.credits -= bill;
  spdlog::info("daily expenses: {:.0f} credits (power+rent+net)", bill);
  // passive income from tech
  state_.credits += state_.passive_income_per_day;
  // market fluctuation
  economy::Market market;
  market.load(catalog_.market_items());
  market.update_prices(*this, state_.market_prices);
  state_.last_market_update_day = day;
  // contract board refresh
  state_.contracts.refresh_offers(*this);
  state_.last_offer_refresh_day = day;
  // remote server heat risk
  for (const auto& rs : state_.remote_servers) add_heat(static_cast<int>(rs.heat_risk * 10));
  // autosave marker
  state_.last_autosave_day = day;
  bus_.publish("game.day", {{"day", day + 1}});
  // weekly salaries
  if (day % 7 == 6) weekly_rollover();
}

void Game::weekly_rollover() {
  double total = 0.0;
  for (auto& e : state_.employees) {
    total += e.salary_weekly;
    e.morale = std::clamp(e.morale - 2, 0, 100);
  }
  state_.credits -= total;
  if (total > 0) {
    spdlog::info("weekly salaries: {:.0f}", total);
    if (state_.credits < 0) {
      // unpaid wages hurt morale
      for (auto& e : state_.employees) e.morale = std::clamp(e.morale - 20, 0, 100);
      state_.credits = 0;
    }
  }
}

void Game::check_heat_thresholds() {
  if (state_.heat >= 76 && !raid_done_) {
    raid_done_ = true;
    // raid: lose a random component or a claimed server
    if (!state_.remote_servers.empty() && rng_.chance(0.5)) {
      auto victim = state_.remote_servers.begin() + rng_.next_int(0, static_cast<int>(state_.remote_servers.size()) - 1);
      spdlog::warn("RAID: lost claimed server {}", victim->name);
      state_.remote_servers.erase(victim);
      bus_.publish("raid.lost_server", json::object());
    } else {
      // destroy a random component
      std::vector<std::pair<hardware::Room*, hardware::Component*>> candidates;
      for (auto& [id, r] : state_.rooms)
        for (auto& c : r.components)
          if (c.online) candidates.emplace_back(&r, &c);
      if (!candidates.empty()) {
        auto& [r, c] = candidates[rng_.next_int(0, static_cast<int>(candidates.size()) - 1)];
        c->online = false;
        c->durability = 0;
        spdlog::warn("RAID: {} in {} was confiscated", c->def_id, r->id);
        bus_.publish("raid.lost_hardware", {{"uid", c->uid}, {"def", c->def_id}});
      }
    }
    state_.reputation -= 10;
    add_heat(-40);
  } else if (state_.heat >= 51 && state_.heat < 76) {
    bus_.publish("raid.warning", {{"heat", state_.heat}});
  }
  if (state_.heat < 50) raid_done_ = false;
}

} // namespace neon::core
