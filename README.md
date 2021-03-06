**Quake 3: Revamp** game code.

To use this you'll need the [Spearmint engine](https://github.com/zturtleman/spearmint).

# New Features
* InstaGib server option
* Assisted Suicides
* Location Damage
* Headshots!
* New HUD layout including new Kill Feed: [Screenshot](https://www.dropbox.com/s/1lkzdh08y7yshtj/Screenshot%20from%202015-05-26%2013%3A14%3A06.png?dl=0)
* Server options for disabling Self-Damage and Fall-Damage
* Other minor tweaks

# New cvars
* g_instaGib: 0 - Normal weapons, 1 - InstaGib Railgun, 2 - InstaGib Rocket Launcher

* g_selfDamage: 0 - No self damage (rocket jumps without penalty), 1 - Self damage

* g_fallDamag: 0 - No fall damage, 1 - Fall damage

* g_weaponMods: 0 - Normal weapons, 1 - A few weapon mods (railgun bounces)

* g_noFootsteps: 0 - Footsteps sounds, 1 - No footstep sounds

#### Help! It errors "CGame VM uses unsupported API" when I try to run it!

The engine and game code are incompatible. Make sure you have the latest of both. Try waiting a couple days and then create an issue for this repo to let the maintainer know.
