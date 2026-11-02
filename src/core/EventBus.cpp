#include "core/EventBus.hpp"

namespace neon::core {

void EventBus::subscribe(const std::string& event, Handler handler) {
  handlers_[event].push_back(std::move(handler));
}

void EventBus::publish(const std::string& event, const nlohmann::json& payload) {
  auto it = handlers_.find(event);
  if (it == handlers_.end()) return;
  for (auto& h : it->second) {
    if (h) h(payload);
  }
}

void EventBus::clear() { handlers_.clear(); }

} // namespace neon::core
