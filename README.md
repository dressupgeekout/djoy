# djoy

djoy is a little toy which interprets a Lua script that lets you specify
event handlers to play sounds. It has support for joysticks and gamepads, so
you can play kinda silly music in your hand like a BopIt.


## Script reference

djoy is essentially a custom Lua 5.3 interpreter with extra capabilities to
play sounds.

Here is the degenerate script, which outlines what all scripts must have at
a minimum:

    -- Degenerate script
    sample_map = { }

    function onbuttondown(ev)
    end

    function onbuttonup(ev)
    end

    function onaxis(ev)
    end

Basically all you have to do is fill in the `sample_map` array with paths to
WAV, FLAC, MP3 and/or Ogg Vorbis files like this:

    sample_map = {
      [0] = "path/to/blah.wav",
      [1] = "path/to/foo.flac",
    }

And then you can play(), loop() and/or stop() these samples by passing in
the number you gave them. For example, play(0) will play blah.wav once, and
stop(1) will stop foo.flac (useful if you had loop()ed it to begin with).

The event structure "ev" that is passed into the event handlers describe
actions of the joysticks.

The table for onbuttondown() and onbuttonup() looks like this:

    {
      timestamp = INTEGER,   -- ms that have elapsed since program start
      which = INTEGER,       -- identifier for the joystick
      button = INTEGER,      -- which button was pushed
    }

The table for onaxis() looks like this:

    {
      timestamp = INTEGER,   -- ms that have elapsed since program start
      which = INTEGER,       -- identifier for the joystick
      axis = INTEGER,        -- which axis this event represents
      value = INTEGER,       -- value between -32768 and 32767
    }

This enables you to precisely specify what gets played when, for example:

    function onbuttondown(ev)
      if ev.button == 7 then play(1) end
    end


## Caveats

At the moment, the WAV files MUST have a sample size of 16 bits and a sample
rate of 44100 Hz.

There is no easy way to figure out what each button's number is without
print()ing everything first.


## Building

djoy is written against SDL2, the SDL2 Mixer package, and Lua 5.3. SDL
should have been built with support for joysticks and/or gamepads. The only
(potentially) crazy thing is that the Makefile is written in BSD make so you
might need to pick up bmake(1).

I'm primarily developing this on OS X with packages managed by pkgsrc (hence
the /opt/pkg junk in the Makefile) but djoy is extremely portable and should
Just Work (TM) pretty much everywhere.


## License

This code is released under a 2-clause BSD-style license. See the LICENSE
file for details.
