#include "economy/Contract.hpp"

namespace neon::economy {

ContractTemplate ContractTemplate::from_json(const json& j) {
  ContractTemplate t;
  t.id = j.at("id").get<std::string>();
  t.type = j.value("type", "courier");
  t.title = j.value("title", t.id);
  t.description = j.value("description", "");
  t.client_faction = j.value("client_faction", "independent");
  t.reward_min = j.value("reward_min", 0.0);
  t.reward_max = j.value("reward_max", 0.0);
  t.duration_hours = j.value("duration_hours", 24);
  t.requirements = j.value("requirements", json::object());
  t.risk = j.value("risk", 0.0);
  t.rep_effects = j.value("rep_effects", json::object());
  t.difficulty = j.value("difficulty", 1);
  return t;
}

json ContractTemplate::to_json() const {
  return json{{"id", id}, {"type", type}, {"title", title}, {"description", description},
              {"client_faction", client_faction}, {"reward_min", reward_min}, {"reward_max", reward_max},
              {"duration_hours", duration_hours}, {"requirements", requirements}, {"risk", risk},
              {"rep_effects", rep_effects}, {"difficulty", difficulty}};
}

json ContractInstance::to_json() const {
  return json{{"uid", uid}, {"template_id", template_id}, {"type", type}, {"title", title},
              {"description", description}, {"client", client.to_json()}, {"reward", reward},
              {"penalty", penalty}, {"deadline_minute", deadline_minute}, {"accepted_minute", accepted_minute},
              {"status", static_cast<int>(status)}, {"progress", progress}, {"requirements", requirements},
              {"risk", risk}, {"rep_effects", rep_effects}, {"route_index", route_index},
              {"courier_started", courier_started}, {"courier_time_h", courier_time_h},
              {"courier_risk", courier_risk}, {"income_accrued", income_accrued}, {"sla_ok", sla_ok},
              {"sla_total", sla_total}};
}

ContractInstance ContractInstance::from_json(const json& j) {
  ContractInstance c;
  c.uid = j.at("uid").get<std::string>();
  c.template_id = j.value("template_id", "");
  c.type = j.value("type", "courier");
  c.title = j.value("title", c.template_id);
  c.description = j.value("description", "");
  if (j.contains("client")) c.client = Client::from_json(j["client"]);
  c.reward = j.value("reward", 0.0);
  c.penalty = j.value("penalty", 0.0);
  c.deadline_minute = j.value("deadline_minute", 0ULL);
  c.accepted_minute = j.value("accepted_minute", 0ULL);
  c.status = static_cast<ContractStatus>(j.value("status", 0));
  c.progress = j.value("progress", 0.0);
  c.requirements = j.value("requirements", json::object());
  c.risk = j.value("risk", 0.0);
  c.rep_effects = j.value("rep_effects", json::object());
  c.route_index = j.value("route_index", -1);
  c.courier_started = j.value("courier_started", false);
  c.courier_time_h = j.value("courier_time_h", 0.0);
  c.courier_risk = j.value("courier_risk", 0.0);
  c.income_accrued = j.value("income_accrued", 0.0);
  c.sla_ok = j.value("sla_ok", 0.0);
  c.sla_total = j.value("sla_total", 0.0);
  return c;
}

} // namespace neon::economy
