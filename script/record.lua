-- record.lua
-- Example of how to record performances

sample_map = {
  [0] = "basskick.wav",
  [1] = "clap.wav",
  [2] = "drone.wav",
  [3] = "drone2.wav",
  [4] = "hihat.wav",
  [5] = "moo.wav",
  [6] = "snare.wav",
  [7] = "tambourine.wav",
}

button_map = {
  [0] = 7,
  [1] = 2,
  [2] = 5,
  [3] = 1,
  [4] = 1,
  [5] = 6,
  [6] = 3,
  [7] = 0,
  [8] = 4,
  [9] = 0,
  [10] = 2,
  [11] = 3,
  [12] = 4,
}

local f = io.open("/tmp/RECORDING", "w")

function onbuttondown(ev)
  sample = button_map[ev.button]
  play(sample)
  f:write(string.format("%d\tBUTTONDOWN\t%d\n", ev.timestamp, sample))
  f:flush()
end

function onbuttonup(ev)
  --f:write(string.format("%d\tBUTTONUP\t%d\n", ev.timestamp, ev.button))
end

function onaxis(ev)
  --f:write(string.format("%d\tAXIS\t%d\t%d\n", ev.timestamp, ev.axis, ev.value))
end
