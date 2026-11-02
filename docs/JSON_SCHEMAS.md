# JSON Schemas (data/*.json)

All files are UTF-8 JSON. Top level is either an array or an object with an
`items` / `nodes` / `templates` array. Every catalog entry requires an `id`.
Unknown fields are ignored; missing required fields cause the catalog load to
fail (the game logs the error and refuses to start with broken data).

## data/hardware.json — `{"items": [HardwareItem]}`

```json
{
  "id": "hw_rack_t2",            // required, unique
  "name": "Consumer Rack 24U",
  "type": "rack",                // rack|server_unit|cpu|ram|storage|psu|fan|cooler|switch|network_card|pdu|ups|gpu|cyberdeck|consumable
  "tier": 2,                     // 1..5
  "price": 2500.0,
  "power_draw": 15.0,            // watts
  "heat_output": 8.0,            // watts
  "durability": 100.0,
  "noise": 20.0,                 // dB
  "stats": { "slots_u": 24, "ghz": 3.6, "gb": 32, "tb": 4, "mbps": 1000,
             "wattage": 750, "cooling_w": 120, "capacity_w": 6000, "hash_mh": 120 },
  "size_u": 2,                   // rack units (server units)
  "slot_type": "cpu",            // which server slot this fits: cpu|ram|storage|psu|fan|nic|pcie
  "compatibility": [],           // empty = universal; else list of server types/ids allowed
  "description": "..."
}
```

## data/tech_tree.json — `{"nodes": [TechNode]}`

```json
{
  "id": "hw2", "name": "Rack Expansion", "description": "...",
  "branch": "Hardware",          // Hardware|Software|Netrunning|Economy|Stealth|Infrastructure
  "tier": 2,
  "cost_credits": 2000.0, "cost_rp": 10.0,
  "prereqs": ["hw1"],
  "effects": [ {"type": "unlock_hardware", "id": "hw_rack_t3"},
               {"type": "stat_multiplier", "stat": "storage", "value": 1.2} ],
  "x": 0, "y": 320                // UI layout coordinates
}
```

Effect `type` values: `credits`, `rp`, `data_shards`, `heat`, `reputation`,
`unlock_hardware`, `unlock_program`, `unlock_contract_type`, `unlock_cyberware`,
`stat_multiplier` (`stat`,`value`), `passive_income`, `damage_hardware`,
`repair_all`, `power_outage`, and additive bonuses named directly
(`max_ram_bonus`, `program_slots`, `shield_bonus`, `ice_damage_bonus`,
`action_points`, `trace_reduction`, `trace_time_bonus`, `detection_reduction`,
`cooling_efficiency`, `power_efficiency`, `heat_reduction`, `contract_slots`,
`employee_slots`, `mining_bonus`, `income_bonus`, `rp_per_day`, `security_rating`,
`bandwidth_bonus`, `storage_bonus`, `cpu_speed_bonus`).

## data/contracts.json — `{"templates": [ContractTemplate]}`

```json
{
  "id": "ct_courier_1", "type": "courier",   // courier|storage|hosting|mining|data_broker|botnet
  "title": "...", "description": "...",
  "client_faction": "independent",
  "reward_min": 300, "reward_max": 600,
  "duration_hours": 6,
  "requirements": {"storage_tb": 10, "bandwidth_mbps": 100, "uptime_pct": 99, "hash_mh": 50, "data_shards": 5},
  "risk": 0.05,                              // heat multiplier on completion
  "rep_effects": {"arasaka": 4},
  "difficulty": 1
}
```

## data/factions.json — `{"items": [Faction]}`

```json
{ "id": "arasaka", "name": "Arasoft", "description": "...", "color": "#4FA3FF",
  "base_rep": 10, "likes": ["hosting"], "dislikes": ["botnet"] }
```

## data/market_items.json — `{"items": [MarketItem]}`

```json
{ "id": "crypto_neon", "name": "NEON coin", "category": "crypto",
  "min_price": 1.0, "max_price": 8.0, "volatility": 0.35,
  "black_market": false, "description": "..." }
```

## data/netrun_programs.json — `{"items": [Program]}`

```json
{ "id": "prog_cloak", "name": "Cloak", "description": "...",
  "ram_cost": 2, "cooldown": 3,
  "effect_type": "cloak",       // cloak|decrypt|attack|heal|data_spike|backdoor|shield_boost|trace_scrub
  "effect_value": 25.0,
  "rarity": "uncommon",         // common|uncommon|rare|epic|legendary
  "lua_script": "cloak.lua"     // optional, resolved under scripts/lua/programs/
}
```

## data/events.json — `{"items": [Event]}`

```json
{ "id": "ev_power_surge", "name": "Power Surge", "description": "...",
  "weight": 3.0,
  "conditions": {"heat_min": 0, "heat_max": 100, "time_of_day": "day|night", "min_rooms": 1},
  "effects": [ {"type": "damage_hardware", "value": 25} ],
  "lua_script": "power_surge.lua"   // optional, resolved under scripts/lua/events/;
                                     // if it returns an effect table it overrides `effects`
}
```

## Save file (`%LOCALAPPDATA%\NeonGrid\saves\savegame.json`)

Top-level fields: `version`, `seed`, `rng_state`, `minute`, `credits`, `rp`,
`data_shards`, `heat`, `reputation`, `rooms` (object keyed by room id: tiles,
components, cables, temperature, SLA counters), `active_room`, `inventory`,
`purchased_tech`, `unlocked_items`, `unlocked_programs`,
`unlocked_contract_types`, `contracts` (offers/active), `faction_rep`,
`market_prices`, `inventory_count`, `employees`, `remote_servers`,
`netrun_targets`, `cyberdeck`, `cyberware`, `stat_bonuses`, `stat_multipliers`,
`passive_income_per_day`, plus bookkeeping counters. `version` is checked on
load; older saves get migrated (see `Game::load` / `SaveLoad.cpp`).
