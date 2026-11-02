#include "netrunning/NetrunSession.hpp"

#include "core/DataCatalog.hpp"
#include "core/Game.hpp"
#include "utils/Random.hpp"

#include <algorithm>
#include <cmath>

namespace neon::netrunning {

NetrunSession NetrunSession::start(neon::core::Game& game, const NetrunTarget& target, bool use_proxy) {
  NetrunSession s;
  s.game_ = &game;
  s.target = target;
  s.map = GridMap::generate(game, target);
  s.px = s.map.entry.first;
  s.py = s.map.entry.second;
  if (auto* n = s.map.at(s.px, s.py)) n->visited = true;

  const Cyberdeck& deck = game.deck_stats();
  s.max_shield = deck.max_shield + static_cast<int>(game.get_stat("shield_bonus"));
  s.shield = s.max_shield;
  s.action_points = s.actions_per_turn();

  // Equip programs: starters + unlocked, limited by RAM and slots.
  s.programs.clear();
  s.ram_used = 0;
  std::vector<std::string> candidates = {"prog_decrypt", "prog_attack"};
  for (const auto& p : game.state().unlocked_programs) candidates.push_back(p);
  for (const auto& pid : candidates) {
    if (static_cast<int>(s.programs.size()) >= deck.program_slots) break;
    const ProgramDef* def = game.catalog().program(pid);
    if (!def) continue;
    if (s.ram_used + def->ram_cost > deck.ram) continue;
    if (std::find(s.programs.begin(), s.programs.end(), pid) != s.programs.end()) continue;
    s.programs.push_back(pid);
    s.ram_used += def->ram_cost;
  }
  s.add_log("jacked in to " + target.name + (use_proxy ? " (via proxy)" : " (direct)"));
  s.used_proxy = use_proxy;
  return s;
}

int NetrunSession::actions_per_turn() const {
  if (!game_) return 2;
  return 2 + static_cast<int>(game_->get_stat("action_points"));
}

double NetrunSession::trace_rate() const {
  if (!game_) return 4.0;
  double rate = 4.0 - game_->get_stat("trace_reduction");
  if (used_proxy) rate *= 0.5;
  rate -= game_->get_stat("trace_time_bonus") * 0.1;
  return std::max(0.5, rate);
}

bool NetrunSession::ice_at(int x, int y) const {
  for (const auto& ice : map.ices)
    if (ice.x == x && ice.y == y) return true;
  return false;
}

ICE* NetrunSession::ice_at(int x, int y) {
  for (auto& ice : map.ices)
    if (ice.x == x && ice.y == y) return &ice;
  return nullptr;
}

int NetrunSession::adjacent_ice_count() const {
  int count = 0;
  for (const auto& ice : map.ices)
    if (std::abs(ice.x - px) + std::abs(ice.y - py) <= 2) ++count;
  return count;
}

const ProgramDef* NetrunSession::program_def(const std::string& id) const {
  if (!game_) return nullptr;
  return game_->catalog().program(id);
}

int NetrunSession::program_cooldown(const std::string& id) const {
  auto it = cooldowns.find(id);
  return it == cooldowns.end() ? 0 : it->second;
}

void NetrunSession::add_log(const std::string& line) {
  log.push_back(line);
  if (log.size() > 200) log.erase(log.begin());
}

bool NetrunSession::move(int dx, int dy) {
  if (!is_running() || action_points <= 0) return false;
  if (std::abs(dx) + std::abs(dy) != 1) return false;
  int nx = px + dx;
  int ny = py + dy;
  NetrunNode* node = map.at(nx, ny);
  if (!node) return false;
  if (ice_at(nx, ny)) {
    add_log("blocked by ICE");
    return false;
  }
  if (node->type == NodeType::ICE) return false; // ice wall without unit also blocks
  px = nx;
  py = ny;
  node->visited = true;
  --action_points;
  add_log("moved to (" + std::to_string(px) + "," + std::to_string(py) + ")");
  if (node->type == NodeType::Exit) {
    add_log("reached exit node - you can jack out safely");
  }
  detection_check();
  return true;
}

void NetrunSession::detection_check() {
  int near = adjacent_ice_count();
  if (near > 0) {
    detection += 8.0 * near;
    if (detection >= 60.0) {
      for (auto& ice : map.ices) ice.alert = true;
      add_log("ICE alerted!");
    }
  }
  if (detection >= 100.0) {
    detection = 100.0;
    // all ICE converge and hit hard
    for (auto& ice : map.ices) {
      ice.alert = true;
      shield -= ice.damage;
    }
    add_log("full detection lock - all ICE slam your deck");
    if (shield <= 0) {
      shield = 0;
      state = State::Lost;
      add_log("deck flatlined. connection severed.");
    }
  }
}

bool NetrunSession::damage_adjacent_ice(double amount) {
  double bonus = game_ ? game_->get_stat("ice_damage_bonus") : 0.0;
  double total = amount + bonus;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (std::abs(dx) + std::abs(dy) != 1) continue;
      ICE* ice = ice_at(px + dx, py + dy);
      if (!ice) continue;
      ice->hp -= static_cast<int>(total);
      ice->alert = true;
      add_log("hit ICE " + ice->uid + " for " + std::to_string(static_cast<int>(total)) + " dmg");
      if (ice->hp <= 0) {
        add_log("ICE " + ice->uid + " crashed");
        // remove it
        for (auto it = map.ices.begin(); it != map.ices.end(); ++it) {
          if (it->uid == ice->uid) {
            map.ices.erase(it);
            break;
          }
        }
        if (NetrunNode* n = map.at(px + dx, py + dy)) n->type = NodeType::Empty;
        return true;
      }
      return true;
    }
  }
  return false;
}

bool NetrunSession::loot_adjacent_data(bool spike) {
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (std::abs(dx) + std::abs(dy) > 1) continue; // current + orthogonal
      NetrunNode* n = map.at(px + dx, py + dy);
      if (!n || n->type != NodeType::Data || n->looted) continue;
      n->looted = true;
      int shards = n->data_value * (spike ? 3 : 1);
      shards_stolen += shards;
      if (spike) credits_stolen += 50 * n->data_value + target.reward_credits / 4;
      detection += spike ? 20.0 : 10.0;
      add_log(std::string(spike ? "data spike: siphoned " : "decrypted ") + std::to_string(shards) + " shards");
      return true;
    }
  }
  return false;
}

bool NetrunSession::use_program(const std::string& program_id) {
  if (!is_running() || action_points <= 0) return false;
  if (std::find(programs.begin(), programs.end(), program_id) == programs.end()) {
    add_log("program not equipped: " + program_id);
    return false;
  }
  if (program_cooldown(program_id) > 0) {
    add_log("program on cooldown: " + program_id);
    return false;
  }
  const ProgramDef* def = program_def(program_id);
  if (!def) return false;

  bool ok = false;
  if (def->effect_type == "cloak") {
    detection = std::max(0.0, detection - def->effect_value);
    for (auto& ice : map.ices) ice.alert = false;
    add_log("cloak engaged: detection -" + std::to_string(static_cast<int>(def->effect_value)));
    ok = true;
  } else if (def->effect_type == "decrypt") {
    ok = loot_adjacent_data(false);
    if (!ok) add_log("no data node in reach");
  } else if (def->effect_type == "attack") {
    ok = damage_adjacent_ice(def->effect_value);
    if (ok) detection += 15.0;
    else add_log("no ICE in reach");
  } else if (def->effect_type == "heal") {
    shield = std::min(max_shield, shield + static_cast<int>(def->effect_value));
    add_log("deck patched: shield +" + std::to_string(static_cast<int>(def->effect_value)));
    ok = true;
  } else if (def->effect_type == "data_spike") {
    NetrunNode* here = map.at(px, py);
    if (here && here->type == NodeType::Root) {
      here->looted = true;
      shards_stolen += 5 + target.difficulty * 2;
      credits_stolen += target.reward_credits;
      state = State::Won;
      add_log("ROOT SPIKED - full control of " + target.name);
      return true; // free action resolution
    }
    ok = loot_adjacent_data(true);
    if (!ok) add_log("no data node in reach");
  } else if (def->effect_type == "backdoor") {
    NetrunNode* here = map.at(px, py);
    if (here && here->type == NodeType::Root) {
      shards_stolen += 2 + target.difficulty;
      credits_stolen += target.reward_credits;
      state = State::Won;
      add_log("backdoor installed at ROOT of " + target.name);
      return true;
    }
    add_log("backdoor requires the ROOT node");
    return false;
  } else if (def->effect_type == "shield_boost") {
    shield = std::min(max_shield, shield + static_cast<int>(def->effect_value));
    detection = std::max(0.0, detection - 5.0);
    add_log("shield boosted");
    ok = true;
  } else if (def->effect_type == "trace_scrub") {
    trace = std::max(0.0, trace - def->effect_value);
    add_log("trace scrubbed: -" + std::to_string(static_cast<int>(def->effect_value)));
    ok = true;
  } else {
    add_log("unknown program effect: " + def->effect_type);
    return false;
  }

  if (ok) {
    --action_points;
    if (def->cooldown > 0) cooldowns[program_id] = def->cooldown;
    detection_check();
  }
  return ok;
}

void NetrunSession::flee() {
  if (!is_running()) return;
  NetrunNode* here = map.at(px, py);
  if (here && here->type == NodeType::Exit) {
    state = State::Fled;
    add_log("jacked out cleanly with the loot");
  }
}

void NetrunSession::end_turn() {
  if (!is_running()) return;
  ++turn;
  // cooldowns tick
  for (auto& [id, cd] : cooldowns)
    if (cd > 0) --cd;
  // trace advances
  trace += trace_rate();
  if (trace >= 100.0) {
    trace = 100.0;
    state = State::Traced;
    add_log("TRACED! corp ICE traced your signal.");
    return;
  }
  // detection decay when no ICE near
  if (adjacent_ice_count() == 0) detection = std::max(0.0, detection - 5.0);
  ice_phase();
  if (state != State::Running) return;
  action_points = actions_per_turn();
}

void NetrunSession::ice_phase() {
  for (auto& ice : map.ices) {
    if (ice.alert) {
      // chase: greedy step toward player
      int dx = (px > ice.x) ? 1 : (px < ice.x ? -1 : 0);
      int dy = (py > ice.y) ? 1 : (py < ice.y ? -1 : 0);
      if (dx != 0 && !ice_at(ice.x + dx, ice.y) && map.in_bounds(ice.x + dx, ice.y) && map.at(ice.x + dx, ice.y)->type != NodeType::ICE) {
        ice.x += dx;
      } else if (dy != 0 && !ice_at(ice.x, ice.y + dy) && map.in_bounds(ice.x, ice.y + dy) && map.at(ice.x, ice.y + dy)->type != NodeType::ICE) {
        ice.y += dy;
      }
      if (std::abs(ice.x - px) + std::abs(ice.y - py) <= 1) {
        shield -= ice.damage;
        add_log("ICE " + ice.uid + " hits you for " + std::to_string(ice.damage));
        if (shield <= 0) {
          shield = 0;
          state = State::Lost;
          add_log("deck flatlined. connection severed.");
          return;
        }
      }
    } else if (!ice.patrol.empty()) {
      // patrol waypoints
      auto [tx, ty] = ice.patrol[ice.patrol_idx % ice.patrol.size()];
      if (tx == ice.x && ty == ice.y) {
        ice.patrol_idx = (ice.patrol_idx + 1) % ice.patrol.size();
        continue;
      }
      int dx = (tx > ice.x) ? 1 : (tx < ice.x ? -1 : 0);
      int dy = (ty > ice.y) ? 1 : (ty < ice.y ? -1 : 0);
      if (dx != 0 && map.in_bounds(ice.x + dx, ice.y) && map.at(ice.x + dx, ice.y)->type == NodeType::Empty && !ice_at(ice.x + dx, ice.y))
        ice.x += dx;
      else if (dy != 0 && map.in_bounds(ice.x, ice.y + dy) && map.at(ice.x, ice.y + dy)->type == NodeType::Empty && !ice_at(ice.x, ice.y + dy))
        ice.y += dy;
      else
        ice.patrol_idx = (ice.patrol_idx + 1) % ice.patrol.size();
      // spotting: if player within 3 tiles, become alert
      if (std::abs(ice.x - px) + std::abs(ice.y - py) <= 3) {
        ice.alert = true;
        detection += 10.0;
        add_log("ICE " + ice.uid + " spotted you!");
      }
    }
  }
}

json NetrunSession::to_json() const {
  json progs = json::array();
  for (auto& p : programs) progs.push_back(p);
  json cds = json::object();
  for (auto& [k, v] : cooldowns) cds[k] = v;
  return json{{"map", map.to_json()},   {"target", target.to_json()}, {"px", px}, {"py", py},
              {"shield", shield},       {"max_shield", max_shield},   {"action_points", action_points},
              {"detection", detection}, {"trace", trace},             {"turn", turn},
              {"state", static_cast<int>(state)}, {"used_proxy", used_proxy}, {"programs", progs},
              {"cooldowns", cds},       {"ram_used", ram_used},       {"shards_stolen", shards_stolen},
              {"credits_stolen", credits_stolen}, {"log", log}};
}

} // namespace neon::netrunning
