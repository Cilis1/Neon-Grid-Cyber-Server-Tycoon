-- Competitor DDoS: your servers struggle; small reputation hit.
return {
  effects = {
    { type = "reputation", value = -2 },
    { type = "credits", value = -100 },
  },
  message = "rival DDoS degraded your uptime",
}
