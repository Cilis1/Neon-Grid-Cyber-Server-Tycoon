-- Black market sale (night only): cheap shards, but heat.
return {
  effects = {
    { type = "credits", value = -400 },
    { type = "data_shards", value = 5 },
    { type = "heat", value = 8 },
  },
  message = "black market hookup: 5 shards for 400 cr (heat +8)",
}
