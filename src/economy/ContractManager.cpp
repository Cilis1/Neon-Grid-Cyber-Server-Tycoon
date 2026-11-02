#include "economy/ContractManager.hpp"

#include "core/DataCatalog.hpp"
#include "core/Game.hpp"
#include "utils/Random.hpp"

#include <spdlog/spdlog.h>

namespace neon::economy {

std::vector<CourierRoute> ContractManager::courier_routes() {
  return {
      {"Direct arterial (fast, risky)", 2.0, 0.18, 0.0},
      {"Backstreets (slow, safe)", 3.5, 0.05, 0.0},
      {"Paid priority lane (fastest, pricey)", 1.2, 0.30, 150.0},
  };
}

int ContractManager::max_active(neon::core::Game& game) const {
  return 3 + static_cast<int>(game.get_stat("contract_slots"));
}

bool ContractManager::can_accept(neon::core::Game& game, const std::string& offer_uid) const {
  const ContractInstance* offer = find_offer(offer_uid);
  if (!offer) return false;
  if (static_cast<int>(active.size()) >= max_active(game)) return false;
  return true;
}

ContractInstance* ContractManager::accept(neon::core::Game& game, const std::string& offer_uid) {
  if (!can_accept(game, offer_uid)) return nullptr;
  for (auto it = offers.begin(); it != offers.end(); ++it) {
    if (it->uid == offer_uid) {
      ContractInstance c = *it;
      c.accepted_minute = game.state().time.minute;
      offers.erase(it);
      active.push_back(c);
      spdlog::info("accepted contract '{}' from {}", c.title, c.client.name);
      game.bus().publish("contract.accepted", {{"uid", c.uid}, {"title", c.title}});
      return &active.back();
    }
  }
  return nullptr;
}

ContractInstance* ContractManager::find_active(const std::string& uid) {
  for (auto& c : active)
    if (c.uid == uid) return &c;
  return nullptr;
}

ContractInstance* ContractManager::find_offer(const std::string& uid) {
  for (auto& c : offers)
    if (c.uid == uid) return &c;
  return nullptr;
}

const ContractInstance* ContractManager::find_active(const std::string& uid) const {
  for (const auto& c : active)
    if (c.uid == uid) return &c;
  return nullptr;
}

const ContractInstance* ContractManager::find_offer(const std::string& uid) const {
  for (const auto& c : offers)
    if (c.uid == uid) return &c;
  return nullptr;
}

void ContractManager::refresh_offers(neon::core::Game& game) {
  offers.clear();
  const auto& templates = game.catalog().contracts();
  auto& rng = game.rng();
  int count = 4 + rng.next_int(0, 3);
  for (int i = 0; i < count; ++i) {
    if (templates.empty()) break;
    const ContractTemplate& t = rng.pick(templates);
    // contract types must be unlocked
    if (!game.is_contract_type_unlocked(t.type)) continue;
    ContractInstance c;
    c.uid = "contract_" + std::to_string(rng.next_u64() % 1000000);
    c.template_id = t.id;
    c.type = t.type;
    c.title = t.title;
    c.description = t.description;
    c.client = Client::generate(game, t.client_faction);
    c.reward = rng.next_int(static_cast<int>(t.reward_min), static_cast<int>(t.reward_max));
    c.penalty = c.reward * 0.5;
    c.deadline_minute = game.state().time.minute + static_cast<std::uint64_t>(t.duration_hours) * 60;
    c.requirements = t.requirements;
    c.risk = t.risk;
    c.rep_effects = t.rep_effects;
    offers.push_back(c);
  }
}

bool ContractManager::start_courier(neon::core::Game& game, const std::string& contract_uid, int route_index) {
  ContractInstance* c = find_active(contract_uid);
  if (!c || c->type != "courier" || c->courier_started) return false;
  auto routes = courier_routes();
  if (route_index < 0 || route_index >= static_cast<int>(routes.size())) return false;
  if (game.state().credits < routes[route_index].cost) return false;
  game.state().credits -= routes[route_index].cost;
  c->courier_started = true;
  c->route_index = route_index;
  c->courier_time_h = routes[route_index].time_h;
  c->courier_risk = routes[route_index].risk;
  c->progress = 0.0;
  spdlog::info("courier run started: '{}' via route {}", c->title, routes[route_index].name);
  return true;
}

void ContractManager::resolve_courier(neon::core::Game& game, ContractInstance& c) {
  auto& rng = game.rng();
  // random event during travel: shakedown / traffic / ambush
  bool disaster = rng.chance(c.courier_risk);
  if (disaster) {
    int roll = rng.next_int(0, 2);
    if (roll == 0) {
      double loss = c.reward * 0.25;
      game.state().credits = std::max(0.0, game.state().credits - loss);
      spdlog::warn("courier shakedown: lost {:.0f} credits", loss);
      c.status = ContractStatus::Failed;
      c.progress = 1.0;
      game.bus().publish("contract.failed", {{"uid", c.uid}, {"reason", "shakedown"}});
      return;
    } else if (roll == 1) {
      c.status = ContractStatus::Failed;
      c.progress = 1.0;
      spdlog::warn("courier ambushed: packet destroyed");
      game.bus().publish("contract.failed", {{"uid", c.uid}, {"reason", "ambush"}});
      return;
    } // roll == 2: close call, but delivery succeeds
  }
  c.status = ContractStatus::Completed;
  c.progress = 1.0;
  game.state().credits += c.reward;
  game.state().reputation += 2;
  if (c.risk > 0) game.add_heat(static_cast<int>(c.risk * 100));
  if (c.rep_effects.is_object())
    for (auto& [faction, delta] : c.rep_effects.items())
      if (delta.is_number()) FactionTable::adjust_rep(game.state().faction_rep, faction, delta.get<int>());
  spdlog::info("courier contract '{}' completed: +{:.0f} credits", c.title, c.reward);
  game.bus().publish("contract.completed", {{"uid", c.uid}, {"reward", c.reward}});
}

void ContractManager::tick(neon::core::Game& game, std::uint64_t minutes) {
  double hours = static_cast<double>(minutes) / 60.0;
  for (auto& c : active) {
    if (c.status != ContractStatus::Active) continue;
    if (c.type == "courier") {
      if (!c.courier_started) {
        if (c.expired(game.state().time.minute)) {
          c.status = ContractStatus::Failed;
          game.state().reputation -= 1;
          game.bus().publish("contract.failed", {{"uid", c.uid}, {"reason", "never started"}});
        }
        continue;
      }
      double elapsed_h = hours;
      c.progress = std::min(1.0, c.progress + elapsed_h / c.courier_time_h);
      if (c.progress >= 1.0) resolve_courier(game, c);
      else if (c.expired(game.state().time.minute)) {
        c.status = ContractStatus::Failed;
        game.state().reputation -= 2;
        game.bus().publish("contract.failed", {{"uid", c.uid}, {"reason", "deadline"}});
      }
    } else if (c.type == "storage" || c.type == "hosting") {
      // accrue income while SLA uptime requirement is met
      double req_uptime = c.requirements.value("uptime_pct", 99.0) / 100.0;
      double uptime = game.room_uptime(game.state().active_room);
      c.sla_total += hours;
      if (uptime >= req_uptime) {
        c.sla_ok += hours;
        double per_hour = c.reward / std::max(1.0, static_cast<double>(c.deadline_minute - c.accepted_minute) / 60.0);
        double gain = per_hour * hours;
        c.income_accrued += gain;
        game.state().credits += gain;
      }
      if (c.expired(game.state().time.minute)) {
        double final_uptime = c.sla_total > 0 ? c.sla_ok / c.sla_total : 0.0;
        if (final_uptime >= req_uptime) {
          c.status = ContractStatus::Completed;
          game.state().reputation += 3;
          if (c.rep_effects.is_object())
            for (auto& [faction, delta] : c.rep_effects.items())
              if (delta.is_number()) FactionTable::adjust_rep(game.state().faction_rep, faction, delta.get<int>());
          game.bus().publish("contract.completed", {{"uid", c.uid}, {"reward", c.reward}});
        } else {
          c.status = ContractStatus::Failed;
          game.state().credits = std::max(0.0, game.state().credits - c.penalty);
          game.state().reputation -= 3;
          spdlog::warn("SLA breached on '{}': penalty {:.0f}", c.title, c.penalty);
          game.bus().publish("contract.failed", {{"uid", c.uid}, {"reason", "SLA breach"}});
        }
      }
    } else {
      // mining / data_broker / botnet: passive contracts that complete at deadline if requirements met
      if (c.expired(game.state().time.minute)) {
        c.status = ContractStatus::Completed;
        game.state().credits += c.reward;
        game.add_heat(static_cast<int>(c.risk * 100));
        game.bus().publish("contract.completed", {{"uid", c.uid}, {"reward", c.reward}});
      }
    }
  }
  // prune finished contracts occasionally (keep last 20 for history)
  if (active.size() > 20) {
    std::vector<ContractInstance> finished;
    std::vector<ContractInstance> keep;
    for (auto& c : active)
      (c.status == ContractStatus::Active ? keep : finished).push_back(c);
    if (finished.size() > 20) finished.erase(finished.begin(), finished.end() - 20);
    active = keep;
    active.insert(active.end(), finished.begin(), finished.end());
  }
}

json ContractManager::to_json() const {
  json offers_j = json::array();
  for (auto& o : offers) offers_j.push_back(o.to_json());
  json active_j = json::array();
  for (auto& a : active) active_j.push_back(a.to_json());
  return json{{"offers", offers_j}, {"active", active_j}};
}

void ContractManager::from_json(const json& j) {
  offers.clear();
  active.clear();
  if (j.contains("offers") && j["offers"].is_array())
    for (auto& o : j["offers"]) offers.push_back(ContractInstance::from_json(o));
  if (j.contains("active") && j["active"].is_array())
    for (auto& a : j["active"]) active.push_back(ContractInstance::from_json(a));
}

} // namespace neon::economy
