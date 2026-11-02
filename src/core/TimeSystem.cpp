#include "core/TimeSystem.hpp"

#include <cstdio>

namespace neon::core {

bool TimeSystem::is_night() const {
  std::uint64_t h = hour();
  return h >= 22 || h < 6;
}

std::string TimeSystem::format() const {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "Day %llu %02llu:%02llu",
                static_cast<unsigned long long>(day() + 1),
                static_cast<unsigned long long>(hour()),
                static_cast<unsigned long long>(minute_of_hour()));
  return buf;
}

std::string TimeSystem::format_clock() const {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%02llu:%02llu",
                static_cast<unsigned long long>(hour()),
                static_cast<unsigned long long>(minute_of_hour()));
  return buf;
}

double seconds_per_minute(SimSpeed speed) {
  switch (speed) {
    case SimSpeed::Paused: return 0.0;
    case SimSpeed::Normal: return 1.0;
    case SimSpeed::Fast: return 0.5;
    case SimSpeed::VeryFast: return 0.25;
  }
  return 1.0;
}

} // namespace neon::core
