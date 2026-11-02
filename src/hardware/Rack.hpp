#pragma once

#include <string>

#include "hardware/HardwareComponent.hpp"

namespace neon::core {
class DataCatalog;
}

namespace neon::hardware {

struct Room;

// Rack helpers: capacity in rack-units (U), used U, add/remove server units.
int rack_capacity_u(const core::DataCatalog& catalog, const Component& rack);
int rack_used_u(const core::DataCatalog& catalog, const Room& room, const Component& rack);
bool rack_add_server(Room& room, Component& rack, const std::string& server_uid);
bool rack_remove_server(Room& room, Component& rack, const std::string& server_uid);

} // namespace neon::hardware
