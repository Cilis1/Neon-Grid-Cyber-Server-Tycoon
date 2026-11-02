#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace neon::core {

// Minimal synchronous event bus. Events are stringly-typed with a JSON payload.
class EventBus {
public:
  using Handler = std::function<void(const nlohmann::json&)>;

  void subscribe(const std::string& event, Handler handler);
  void publish(const std::string& event, const nlohmann::json& payload = {});
  void clear();

private:
  std::map<std::string, std::vector<Handler>> handlers_;
};

} // namespace neon::core
