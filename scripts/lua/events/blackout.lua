-- City-wide blackout: everything loses power until the breaker is reset.
return {
  effects = { { type = "power_outage", value = 1 } },
  message = "CITY BLACKOUT - breakers tripped!",
}
