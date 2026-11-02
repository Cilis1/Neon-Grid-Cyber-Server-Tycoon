#pragma once

#include <functional>
#include <string>

#include "hardware/ServerRoom.hpp"

namespace neon::core {
class DataCatalog;
}

namespace neon::hardware {

// Build a runtime component instance from a catalog definition.
Component make_component(const std::string& def_id, const std::string& uid);

// Create the starting server room: 16x12 grid, one rack + PDU + cooler +
// switch, one low-end server with starter parts, pre-cabled.
// `uid_gen` produces unique uids (called as uid_gen("prefix")).
Room create_starter_room(const core::DataCatalog& catalog, const std::function<std::string(const std::string&)>& uid_gen);

// Add a cable between two components/ports if not already present.
Cable& add_cable(Room& room, const std::function<std::string(const std::string&)>& uid_gen,
                 const std::string& from_uid, const std::string& from_port,
                 const std::string& to_uid, const std::string& to_port);

} // namespace neon::hardware
