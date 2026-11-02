#include "utils/Random.hpp"

namespace neon::utils {

std::uint64_t Rng::next_u64() {
  std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

double Rng::next_double() {
  return static_cast<double>(next_u64() >> 11) * (1.0 / 9007199254740992.0);
}

int Rng::next_int(int lo, int hi) {
  if (hi <= lo) return lo;
  return lo + static_cast<int>(next_u64() % static_cast<std::uint64_t>(hi - lo + 1));
}

bool Rng::chance(double p) {
  return next_double() < p;
}

} // namespace neon::utils
