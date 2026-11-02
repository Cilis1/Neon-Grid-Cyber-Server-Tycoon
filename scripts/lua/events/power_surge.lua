-- Power surge: damages a random component unless the player has UPS tech.
-- Demonstrates reading ctx and returning effects.
if (ctx.has_ups or false) then
  return { effects = { { type = "credits", value = -50 } }, message = "UPS absorbed the surge (-50 cr wear)" }
end
return {
  effects = {
    { type = "damage_hardware", value = 30 },
    { type = "heat", value = 0 },
  },
  message = "power surge fried a component",
}
