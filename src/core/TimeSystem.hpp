#pragma once

#include <cstdint>
#include <string>

namespace neon::core {

// In-game time: 1 real second = 1 in-game minute at 1x speed.
// One in-game day = 1440 minutes.
class TimeSystem {
public:
  static constexpr std::uint64_t MINUTES_PER_DAY = 1440;
  static constexpr std::uint64_t MINUTES_PER_HOUR = 60;

  // Total in-game minutes since game start (day 0, 00:00).
  std::uint64_t minute = 0;

  std::uint64_t day() const { return minute / MINUTES_PER_DAY; }
  std::uint64_t hour() const { return (minute / MINUTES_PER_HOUR) % 24; }
  std::uint64_t minute_of_hour() const { return minute % MINUTES_PER_HOUR; }
  std::uint64_t minute_of_day() const { return minute % MINUTES_PER_DAY; }

  bool is_night() const; // 22:00 - 06:00
  std::string format() const;             // "Day 3 14:05"
  std::string format_clock() const;       // "14:05"

  void advance(std::uint64_t minutes = 1) { minute += minutes; }
};

enum class SimSpeed { Paused = 0, Normal = 1, Fast = 2, VeryFast = 4 };

// Real seconds per in-game minute at a given speed.
double seconds_per_minute(SimSpeed speed);

} // namespace neon::core
