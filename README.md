# Inspo Generator v2

A procedural decoration engine for the Geometry Dash editor (Geode mod). One click builds a complete, animated scene around your camera: layered structures, a parallax background and a matching color palette. Everything is made from normal objects and triggers, so you can take it apart, edit it, or keep it.

## Editor toolbar

| Button | What it does |
|---|---|
| **Inspire** | Generate a new batch around the camera |
| **Reroll** | Same colors, new shapes |
| **Recolor** | Same shapes, new palette |
| **< > ^ v** | Move the whole last batch (objects and triggers together) |
| **Delete** | Remove the last batch |
| **...** | Settings |

The batch is selected after generating, so GD's own move, rotate, scale and copy tools work on it too.

## What it builds

**Structures**
- **Citadel**: tiered tower with lit/shaded faces, arched windows that flicker, corbels, crenellations, banners, side turrets and a spire with a spinning halo
- **Crystals**: two-tone faceted crystals with glowing cores on a rock mound, orbiting shards and twinkling sparks
- **Floating island**: strata-banded rock with a lit face, glowing roots, an optional waterfall, and a tower, crystals or tree on top; it bobs
- **Mech core**: meshing gears (correct speed ratios), a segmented outer ring, a tick ring, pumping pistons, an energy core and orbiters
- **Portal**: pillars with runes, braziers, a voussoir arch with keystone gem, an energy doorway, a spinning rune ring and rising particles
- **Orbital**: banded planet with shadow and rim light, a 3D ring (back half behind, front half in front) and orbiting moons
- **Tree**: recursive branching with blossom clusters, a gentle sway from the roots and falling petals
- **Colonnade**: columns, arches, dentil beam, a pediment or balustrade, and swinging lanterns

**Background**: dithered sky gradient, twinkling stars, a moon or sun with halo, a far mountain range or city skyline, mid hills with trees or spires, foreground rubble, drifting fog and floating motes. Layers parallax at different speeds.

**Colors**: 10 themes × 4 harmony rules, with 20 color roles (dark/mid/light fill, edge, accent, additive glow, windows, energy, sky bands, silhouettes, fog, and more). Each role is its own color channel, so you can retune the whole scene by editing a few channels.

**Motion**: rising/dropping entrance animations as the player approaches, bobbing, spinning, swaying, flickering, color pulses, particles and parallax. All of it uses move, rotate, alpha, pulse, color and spawn triggers in looping chains.

## Settings
Theme, color harmony, structure type (mix or one type), structures per batch, detail level (1–3), motion (static / ambient / full), background, parallax, entrance animations, BG/ground/line colors, a **seed** (type any number or word to reproduce a batch exactly), move step size, the lowest group ID, the first color channel (uses 20 from there, default 970–989), and the solid object ID.

Group IDs are allocated above the highest group already used in your level, so they never touch your own groups.

## Building
**Easiest:** push this folder to a GitHub repo. The included workflow builds Windows, macOS and Android. Download the `.geode` from the run's *Build Output* artifact and put it in `geode/mods`.

**Locally:** install the [Geode CLI + SDK](https://docs.geode-sdk.org/getting-started/), then run `geode build` here.

Targets Geode 5.11 / GD 2.2081. All game functions and fields used were checked against the current Geode bindings.

## Notes and known limits
- **Colors in the editor:** the mod writes the palette into the level's start colors so you see it right away. This changes the level's BG/ground/line too, unless "Set BG / ground / line colors" is off. If that part ever fails to compile on a future Geode, build with `-DINSPO_EDITOR_COLORS=OFF`.
- **Entrance animations** start structures offset and move them in when the player gets close. If you test from a start position after a structure's trigger, that structure stays hidden. Turn off "Entrance animations" or start earlier.
- **Parallax** uses move triggers locked to player X and is timed for normal (1x) speed.
- **Solid object:** all shapes use object 211 (a plain colorable square). If shapes come out textured, set "Solid square object ID" to another plain square.
- **Size:** a 3-structure batch at detail 2 is roughly 3–5k objects. Use detail 1 or fewer structures for lighter scenes.

## tools/
`preview.cpp` + `render.py` run the generator outside the game and render a PNG approximation. Useful for tweaking the generators without launching GD:
```
g++ -std=c++20 -O2 -o preview tools/preview.cpp src/core/Engine.cpp src/gen/*.cpp
./preview 42 sunset mix 4 > level.txt
python3 tools/render.py level.txt out.png
```
