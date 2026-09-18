# Fourfold

**A four-element bending combat demo in Unreal Engine 5.8: four elements built on the Gameplay Ability System in C++, themed habitats, an AI sparring duel, and a Python toolchain that drives the editor remotely for testing, mocap import, and capture.**

## What it is

Fourfold is the game's name; the Unreal module, classes and project file keep their original internal prefix `ATLA`, which predates the name.

A third-person arena game where the player bends one of four elements, each with its own resource model and a ladder of techniques from a spammable basic attack up to a signature form:

- **Water** (Tai Chi): a carried Water meter refilled by channeling from pools in the level (or pinned at max while standing at the source). Water Whip, Ice Spears, Ice Wall, Octopus Form (orbiting tendrils that lash and grab-throw), Draw channel.
- **Earth** (Hung Gar): positional ammunition; Earth regenerates only while standing on bendable ground and every earth ability is refused mid-air. Rock Jab / charged Boulder, Earth Spikes, launchable Earth Wall, Earth Armor (conforming plates, 40% damage reduction), Earth Launch column, Boulder Hoist, and a metalbending unlock.
- **Fire** (Northern Shaolin): runs on chi. Fire Jab / Blast, Fire Lash, projectile-incinerating Wall of Flame, Inferno Nova, chainable Fire Jet (hover glide), Breath of Fire, and Lightning on a held charge.
- **Air** (Baguazhang): cheapest costs, lightest hits, most mobility. Air Blast, Air Swipe, projectile-shredding Wind Dome, Cyclone, rideable Air Scooter and Air Spout, Updraft glide, plus jump and air-control passives.

Around the kits: signature forms grant a 10 s Empowered state (+50% damage, bigger projectiles); idle element auras; source-draw streams into every cast wind-up; structure damage so boulders siege ice walls; per-element impact bursts. Four **habitats** (arctic, quarry, volcanic field, sky temple) expanded into biome rings with ambient creatures (koi, badgermole, dragons, sky bison), plus a pro-bending **coliseum** option. Tab switches element kit and teleports between habitats; N travels to the coliseum.

The **AI duel** is a sparring opponent (`AATLABenderAI`, an `AAIController`) that keeps fighting range, strafes, and casts its element's techniques on a human-ish rhythm. B toggles it; the duel runs health to zero with on-screen meters and a knockdown finish.

**C++ / Blueprint split.** Gameplay is C++ (`Source/ATLA`): the ability system component, attribute set (Health, Chi, Water, Earth), native gameplay tags, cost/cooldown/damage `UGameplayEffect` subclasses, a `UATLACastAbility` base that plays a montage and fires `OnCast()` at the release moment, one cosmetic actor per technique, the habitat actors, and the AI controller. Blueprint/OFPA content is the Epic third-person template character and mannequins, two upper-body montages, generated materials, and the arena map. Animation comes from CMU mocap clips retargeted onto the mannequin and played as dynamic montages in windows chosen by measurement (see `Design/anim-windows.md`).

**Editor automation.** Every feature was verified without hand-testing: `Tools/ue_run.py` sends a script into the running editor over the Python remote-execution channel, the script starts Play-In-Editor, drives the player character, asserts damage/cost/position numbers, takes screenshots, and self-disarms when PIE ends. The same channel runs the mocap import and IK-retarget pipeline and the portfolio reel capture.

There is a gameplay reel on my portfolio. <!-- TODO: portfolio reel link -->

## Why I built it

I wanted a systems-heavy Unreal project that exercised the Gameplay Ability System end to end (attributes, effects, tags, replicated actors) rather than a single mechanic, and Avatar's four bending styles gave a natural way to design four kits that share one framework but feel different in resource logic, rhythm, and counterplay. Waterbending was built first as the reference framework; each other element was written as a design spec extrapolated from it and then implemented against that spec. The Python tooling grew out of needing to verify each iteration numerically and reproducibly on a memory-constrained Mac without babysitting the editor.

## Status

As of 2026-09-15:

**Done**
- All four element kits implemented and gauntlet-verified (water 2026-08-03, earth 2026-08-06, fire and air 2026-08-13), with element loadout switching, structure damage, empowered state, and element auras.
- Four habitats with biome dressing and ambient creatures, the coliseum, and per-zone sparring dummies.
- AI sparring opponent and duel-to-zero minigame with health meters and knockdown finish.
- CMU mocap pipeline: Blender conversion, batch import, auto IK rigs, batch retarget, measured window selection; real martial-arts clips on every cast ability and a cartwheel dodge roll.
- Firebending fully on Niagara (bolts, wall, nova, jet, breath, lightning sparks, fist aura, map-load pre-warm); Niagara ambience in three zones; ribbon trails on water/air; dirt bursts on earth impacts.
- Portfolio reel tooling: scripted 50 s playthrough captured frame-by-frame and assembled with ffmpeg.

**In progress / open**
- Motion-matching locomotion from the Game Animation Sample is staged behind a flag (plugins enabled, trajectory component wired) but not yet driving movement.
- Remaining Niagara migration for water, earth, and air signature effects (dome, cyclone, scooter, octopus tendrils) needs hand-authored systems.
- Two-player listen-server test; actors are replicated but multiplayer has not been exercised.
- Work is on the `anim-system-rebuild` branch.

## Stack

- Unreal Engine 5.8, C++ game module `ATLA` (Runtime), Enhanced Input, UMG, Niagara.
- Plugins (from `ATLA.uproject`): GameplayAbilities, StateTree, GameplayStateTree, PoseSearch, Chooser, MotionTrajectory, AnimationWarping, AnimationLocomotionLibrary, ModelingToolsEditorMode (editor only).
- Python 3 editor scripting through the PythonScriptPlugin remote-execution channel (`bRemoteExecution=True` in `Config/DefaultEngine.ini`); Blender (headless) for FBX conversion; ffmpeg for reel assembly.
- Tested on macOS (Apple Silicon, Metal).

## Opening the project

Requirements: Unreal Engine 5.8 with the C++ toolchain (Xcode on macOS, where this was developed and tested), Python 3 on the host for `Tools/`.

1. Clone the repo. If you are cloning the Git LFS snapshot, run `git lfs pull` so the `.uasset`/`.umap`/`.fbx` files are real binaries.
2. Re-add the third-party content packs before opening the maps. They are not in this repo; the arena map and several actors reference Niagara systems and animations from them, so without the packs those references resolve to nothing and you will see missing-asset warnings and placeholder visuals. `THIRD_PARTY.md` lists each pack, where it comes from, and how to restore it (Fab library: Window -> Fab -> Add to project).
3. Open `ATLA.uproject` (or double-click `OpenATLA.command`, which launches the editor without the crash-reporter helper) and let it compile the module. `Tools/play_standalone.sh` launches the game without the editor.
4. The engine path in `OpenATLA.command`, `Tools/play_standalone.sh`, `Tools/ue_run.py`, and `project.yml` defaults to the Epic Games Launcher location on macOS (`/Users/Shared/Epic Games/UE_5.8`); set `UE_ROOT` to override it for the scripts.

Controls (keyboard/mouse): LMB basic (hold for auto or charge), Q / E / R / F techniques, RMB draw or channel, Shift dodge (double for roll), Tab switch element and habitat, N coliseum, B toggle the AI opponent.

## Tools

All Python scripts except the runner and the Blender script import `unreal` and run inside the editor via `python3 Tools/ue_run.py Tools/<script>.py`.

- `ue_run.py`, `ue_wait_and_run.py`: host-side runner. Discovers the editor over the remote-execution multicast channel (with macOS loopback patches), sends a script, prints its log output; the wait variant polls until the editor is reachable.
- Verification harnesses (`*_verify.py`, `*_probe.py`, `gauntlet_*.py`, `element_anim_gauntlet.py`, `*_trace.py`, `fire_pass*_test.py`, `speed_integrity.py`, `zone_census.py`, ...): start PIE, switch element, cast, and assert costs, damage, positions, and animation health; screenshot to `Saved/`.
- Animation measurement (`anim_strike_scan.py`, `anim_move_scan.py`, `mixamo_window_scan.py`, `uppercut_scan.py`, `stomp_scan.py`, `allfours_scan.py`, `bone_diff.py`, `clip_motion_check.py`, `pose_sheet.py`, `pose_shot.py`, `preview_anim.py`): evaluate bone poses offline to find strike peaks and render candidate frames so montage windows are picked from data.
- Import and retarget (`blender_cmu_convert.py` runs in Blender; `import_cmu*.py`, `retarget_cmu.py`, `fix_retarget*.py`, `fix_taichi*.py`, `import_mixamo.py`, `build_mixamo_retarget.py`, `build_gasp_locomotion.py`): convert and import FBX clips, build auto IK rigs and retargeters, batch-retarget onto the game skeleton.
- Content builders (`build_mats.py`, `build_mats2.py`, `build_zones.py`, `dress_volcano.py`, `dress_arctic_temple.py`, `inventory_packs.py`): generate materials and zone geometry, place Niagara ambience, and dump the installed pack inventory to `Design/vfx-inventory.md`.
- Reel capture (`reel_setup.py`, `reel_choreo.py`, `reel_assemble.sh`): prepare a fixed-size PIE window, run the scripted four-element playthrough with one `Shot` per frame, then drop editor frames and encode with ffmpeg.
- Launchers (`play_standalone.sh`, `OpenATLA.command` at the root).

## Layout

```
ATLA.uproject           Module + plugin list
Source/ATLA/            C++ game module
  Bending/              GAS abilities, attribute set, tags, effects, technique actors, HUD
  AI/                   ATLABenderAI sparring opponent
  World/                Habitat zones, creatures, coliseum
  Variant_*/            Epic third-person template variants (combat, platforming, side-scrolling)
Content/                Blueprints/OFPA content: mannequins, map, Bending/ materials and montages,
                        Anims/CMU + CMU_Manny + Retarget (ours); third-party packs excluded
Import/CMU/             CMU mocap FBX (raw + Blender-converted) with MANIFEST.md
Config/                 Default*.ini (remote execution enabled, background throttle off)
Design/                 Element specs, animation-window manifest, art plan and sourcing notes
Tools/                  Python editor scripts and shell helpers (see Tools)
GOALS.md, build_log.md  Project pillars and dated build log
project.yml             Manifest for a local dev dashboard (machine-specific engine path)
THIRD_PARTY.md          Excluded packs, licences, restore steps, attributions
```

## License

Code (`Source/`, `Tools/`, `Config/`) is MIT, see `LICENSE`. My own content under `Content/` (Bending materials and montages, the CMU-derived animation assets and retarget rigs, the arena map) is MIT as well. Third-party content is excluded from this repository and listed in `THIRD_PARTY.md`; Epic template content that is included stays under the Unreal Engine EULA content terms, and the CMU mocap data carries the attribution given there.
