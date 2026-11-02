#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace neon::utils {

// Deterministic, serializable RNG (SplitMix64). State is a single uint64 so it
// can be stored directly inside the JSON save file.
class Rng {
public:
  Rng() = default;
  explicit Rng(std::uint64_t seed) : state_(seed == 0 ? 0x9E3779B97F4A7C15ULL : seed) {}

  std::uint64_t next_u64();
  // [0, 1)
  double next_double();
  // inclusive [lo, hi]
  int next_int(int lo, int hi);
  // true with probability p
  bool chance(double p);
  template <typename T>
  const T& pick(const std::vector<T>& v) {
    return v[static_cast<std::size_t>(next_int(0, static_cast<int>(v.size()) - 1))];
  }

  std::uint64_t state() const { return state_; }
  void set_state(std::uint64_t s) { state_ = s; }

private:
  std::uint64_t state_ = 0x853C49E6748FEA9BULL;
};

} // namespace neon::utils
