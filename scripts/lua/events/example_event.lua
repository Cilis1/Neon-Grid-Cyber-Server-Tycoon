-- Example event script: runs when the event with lua_script="example_event.lua" fires.
-- `ctx` is a table with event context (event_id, credits, heat, ...).
-- Return a table of effects to apply, e.g. { effects = { { type="credits", value=100 } } }
-- or a flat effect { type = "credits", value = 100 }.
-- If the script returns nothing, the C++ effect list from events.json is used instead.

local shards = 2 + (ctx.difficulty or 0)
return {
  effects = {
    { type = "credits", value = 150 },
    { type = "data_shards", value = shards },
  },
  message = "example lua event fired: +150 credits, +" .. shards .. " shards",
}
