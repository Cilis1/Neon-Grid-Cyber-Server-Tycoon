-- Cloak program behavior (overrides/augments the built-in effect when present).
-- ctx contains: program id, detection, trace, turn, target difficulty.
local reduction = 25 + (ctx.difficulty or 0) * 2
return {
  detection_delta = -reduction,
  reset_alert = true,
  message = "cloak engaged: detection -" .. reduction,
}
