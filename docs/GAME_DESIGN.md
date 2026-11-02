# Game Design Document — Neon Grid: Cyber Server Tycoon

## Pitch

You are a netrunner and server admin in a neon-drenched megacity. Starting from
a single rented rack in a basement, you run legal (and not-so-legal) services —
data courier, storage rental, hosting, crypto mining, data brokering — while
upgrading hardware, expanding rooms, researching tech, and hacking other servers
in turn-based netruns. The goal: a distributed server empire that dominates the
city's digital underground.

## Core loop

1. **Inspect** the server room (power/heat/temperature gauges, component wear).
2. **Accept contracts** (courier, storage, hosting, mining, brokering, botnet).
3. **Buy/install/upgrade hardware** (shop → inventory → rack slots / tiles → cables).
4. **Netrun** targets to steal data/credits and claim abandoned servers.
5. **Manage** heat, reputation, power, cooling, dust, employees.
6. **Research** the tech tree (credits + RP).
7. **Expand** rooms, hire staff, install cyberware.
8. Repeat with rising complexity and risk.

## Systems

### Server room & hardware
- 16×12 tile grid per room (expandable). Tiles hold racks, coolers, PDUs, UPS,
  switches. Racks have U capacity; server units (1U/2U/4U) go into racks and have
  slots: cpu, ram, storage, psu, fan, nic, pcie (GPUs/ASICs).
- Every component: power draw, heat output, durability, dust, wear, noise, tier.
- **Power**: draw must be ≤ PDU/UPS capacity or components shut down randomly /
  the breaker trips (reset manually). **Cooling**: heat must be ≤ cooling
  capacity (fans + room coolers + ambient, reduced by dust) or temperature rises;
  >70 °C causes failures, >45 °C derates performance (to 0.4× at 85 °C).
- **Cables**: power cables (yellow) to PDUs, net cables (cyan) to switches —
  drawn as bezier curves; uncabled hardware is offline.
- **Maintenance**: dust accumulates (~full in 33 h), wear on running servers;
  clean with canned air, repair for credits, hire Maintenance Techs to automate.
- **Tiers**: 1 scavenged → 5 military/experimental. Higher tiers unlock via the
  tech tree (`unlock_hardware`).

### Netrunning
- Turn-based grid (12×8). Nodes: Entry, Data, ICE, Control, Exit, Root.
- Each turn: 2 action points (+bonuses) — move 1 tile and/or use 1 program.
- Programs (RAM cost + cooldown, Lua-scriptable): Decrypt, Attack, Cloak, Heal,
  Data Spike, Backdoor, Shield Boost, Trace Scrub, Worm, Daemon.
- ICE patrols waypoints, alerts within 3 tiles, chases and damages your shield.
  Detection ≥ 100 = ICE slam; shield 0 = run lost.
- Trace fills each turn (slower with proxies/Ghost Protocol); at 100 % you're
  traced → heat + fines. Reach ROOT and use Backdoor (win) or Data Spike (win,
  more loot). At an Exit node you can flee and keep the loot.
- Targets: abandoned (low sec, claimable → passive income + proxy), corporate
  (high sec, big rewards, heat), faction (rep effects).

### Tech tree
- 54 nodes, 6 branches × tiers 1–5, DAG prereqs, costs in credits + RP.
- RP from research-lab tech (`rp_per_day`), contracts, netrun data.
- Effect types: `unlock_hardware`, `unlock_program`, `unlock_contract_type`,
  `unlock_cyberware`, `stat_multiplier`, additive bonuses (`max_ram_bonus`,
  `cooling_efficiency`, `power_efficiency`, `heat_reduction`, `trace_time_bonus`,
  `trace_reduction`, `detection_reduction`, `ice_damage_bonus`, `shield_bonus`,
  `action_points`, `contract_slots`, `employee_slots`, `mining_bonus`,
  `income_bonus`, `rp_per_day`, …), `passive_income`, instant `credits`/`rp`/`data_shards`.

### Economy
- Credits (₡) + Data Shards. Income: courier contracts (route minigame: 3 routes
  with time/risk/cost tradeoffs + random events), storage/hosting SLAs (uptime-
  tracked passive income), mining (hash power × NEON price, fluctuates daily),
  data brokering, botnet rental, claimed-server income, tech passive income.
- Expenses: daily power bill (kWh × rate), rent per room, internet, weekly
  salaries, hardware, repairs, bribes.
- Market prices re-roll daily within `[min_price, max_price]` scaled by
  volatility; black market items (marked) add heat.
- 5 factions (Arasoft, Glitch Collective, Neon Serpents, CityNet Security,
  The Loa) with −100…+100 rep affecting offers and events.

### Heat & security
- Heat 0–100. +heat from illegal runs, botnets, black market, failed jobs;
  decays 1/hour; reduce with bribes/proxies/security staff.
- Thresholds: 26–50 police scans; 51–75 raid warning (bribe to calm);
  76–100 raid (lose a component or claimed server, rep −10).
- 16 random events every ~2 in-game hours (power surge, blackout, rival DDoS,
  netrunner attack on you, market moves, …), some with Lua scripts.

### Employees & cyberware
- Hire from a job board: Maintenance Tech (auto-clean), Netrunner (auto data),
  Contract Manager (auto-accept couriers), Security Specialist (heat ↓).
  Skill 1–5, weekly salary, morale. Max = 2 + tech bonuses.
- Cyberware (ripperdoc market / tech): Neural Link +4 RAM, Reflex Tuner +1 AP,
  ICE Breaker +8 dmg, Ghost Protocol −1.5 trace, Dermal Plating +25 shield.

### Time & saves
- 1 real second = 1 in-game minute at 1× (pause/1×/2×/4×). Day = 1440 min.
  Fixed-timestep accumulator, capped catch-up ticks.
- JSON saves with `version` + migrations; autosave each in-game day;
  `%LOCALAPPDATA%\NeonGrid\saves\savegame.json`. `--reset` wipes the save.

## Balancing notes

- Starter: ₡10,000, 25 RP, one 12U rack, 3 kW PDU, one 1U server (~200 W draw).
- First research (500 ₡ / 5 RP) unlocks the tier-2 rack; courier contracts
  (~₡300–600 per 2–3.5 h run) fund the first upgrades.
- Mining is a mid-game power/heat sink; overloading the starter PDU is the
  classic early mistake — buy a tier-2 PDU (research `inf2`) or a cooler.

## Roadmap

- Richer courier city map with more nodes/edges; day/night price effects (partially in).
- NPC netrunner defense minigame on your own servers (currently a stat check event).
- SQLite-backed leaderboards, mod support via `scripts/lua/` + `data/` overrides.
- Soundtrack & SFX (SDL2_mixer is wired up; assets pending).
