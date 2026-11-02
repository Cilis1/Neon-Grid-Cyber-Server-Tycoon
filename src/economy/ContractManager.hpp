#pragma once

#include <string>
#include <vector>

#include "economy/Contract.hpp"

namespace neon::core {
class Game;
}

namespace neon::economy {

// A route option for the courier minigame.
struct CourierRoute {
  std::string name;
  double time_h = 2.0;
  double risk = 0.1; // probability of a bad event
  double cost = 0.0; // bribe/toll cost
};

// Owns the contract board: daily-refreshed offers + active contracts + tick logic.
class ContractManager {
public:
  std::vector<ContractInstance> offers;        // available to accept (refreshed daily)
  std::vector<ContractInstance> active;        // accepted contracts

  static std::vector<CourierRoute> courier_routes();

  // Regenerate the daily offer board.
  void refresh_offers(neon::core::Game& game);

  int max_active(neon::core::Game& game) const;
  bool can_accept(neon::core::Game& game, const std::string& offer_uid) const;
  // Accept an offer: moves it into `active`. Returns pointer or nullptr.
  ContractInstance* accept(neon::core::Game& game, const std::string& offer_uid);

  // Start the courier minigame: choose a route.
  bool start_courier(neon::core::Game& game, const std::string& contract_uid, int route_index);

  // Advance all active contracts by `minutes` in-game minutes.
  void tick(neon::core::Game& game, std::uint64_t minutes);

  ContractInstance* find_active(const std::string& uid);
  ContractInstance* find_offer(const std::string& uid);
  const ContractInstance* find_active(const std::string& uid) const;
  const ContractInstance* find_offer(const std::string& uid) const;

  // Resolve a finished courier run (called by tick when progress completes).
  void resolve_courier(neon::core::Game& game, ContractInstance& c);

  json to_json() const;
  void from_json(const json& j);
};

} // namespace neon::economy
