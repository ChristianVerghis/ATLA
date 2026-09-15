# Third-party content

This repository contains the project's code, configuration, design docs, tooling, and the content the author made or derived from freely redistributable data. Everything below is third-party content that the project uses but that is **not committed** (each path is in `.gitignore`). The maps and several actors reference assets from these packs, so restore them before opening the project or expect missing-asset warnings and placeholder visuals.

Two reasons a pack is excluded:

- **Licence.** Fab Standard License / Epic Content License content and Mixamo animations may be used inside a project and shipped in a compiled build, but the raw assets may not be redistributed on their own. A public source repository would be exactly that.
- **Size.** Several packs are hundreds of megabytes to multiple gigabytes each (the two Paragon packs are about 2.7 GB each; the Game Animation Sample library is about 2.7 GB).

## Restoring Fab packs

1. Sign in to fab.com with the Epic account that claimed the packs (the free listings are permanent library entries once claimed).
2. In the editor: Window -> Fab -> sign in -> find the pack in your library -> **Add to project**. On macOS, if the button misbehaves, use Epic Games Launcher -> Fab Library -> Add to project instead.
3. Confirm the pack lands in the `/Game/...` folder listed below; the project references assets by those paths.

## Fab packs (Niagara VFX)

| Pack | Source | Path in project | Used for |
|---|---|---|---|
| Niagara Examples Pack | Epic Games, Fab (free) | `Content/NiagaraExamples/` | Dirt explosions on earth impacts, footstep and misc systems, ambience |
| Vefects Free Fire VFX | Vefects, Fab (free) | `Content/Vefects/` | `NS_Fire_Small` / `NS_Fire_Medium` on fire bolts, Wall of Flame, fist aura |
| Realistic Fire & Explosion Vol.1 Free Starter | Fab (free) | `Content/Fire_EXP_Vol01_Free/` | Impact detonations, nova explosion |
| M5 VFX Vol2: Fire and Flames | M5 VFX, Fab (free) | `Content/M5VFXVOL2/` | Fire and ember systems |
| Sparks Embers | Fab (free) | `Content/Sparks_Embers/` | Lightning crackle and strike shrapnel, volcanic embers |
| VfxSTOCK Stylish Fire VFX | VfxSTOCK, Fab (free) | `Content/Stylish_Fire_VFX/` | Stylised fire systems |
| Fab plugin import folder | Fab | `Content/Fab/` | Where the in-editor Fab browser drops imports |

## Fab packs (animation and characters)

| Pack | Source | Path in project | Used for |
|---|---|---|---|
| MC Sample Animation Pack | Fab (free) | `Content/MC_Sample/` | Reference and candidate combat clips |
| MoCap Online: MCO Mocap Basics (Free Animation Pack) | MoCap Online, Fab (free) | `Content/MCO_Mocap_Basics/` plus the source-file half the pack ships alongside the uassets: `Source/FBX/`, `Source/Maya/`, `Source/Motionbuilder/`, `Source/SKMannequin_A_Pose/`, `Source/SKMannequin_T_Pose/`, `Source/SK_Mannequin_*_README.txt`, `Source/UE4_MocapOnline_Tech_Notes.txt`, and the store thumbnail `MCO_Mocap_Basics.png` | Mobility / idle basics; the FBX, Maya and MotionBuilder templates are the pack's editing sources on the UE4 mannequin |
| Paragon: Gideon | Epic Games, Fab (free) | `Content/ParagonGideon/` | Caster animations (candidates for bending forms) |
| Paragon: Aurora | Epic Games, Fab (free) | `Content/ParagonAurora/` | Ice-themed caster animations |
| UEFN Mannequin | Epic Games (ships with the Game Animation Sample) | `Content/Characters/UEFN_Mannequin/` | Skeleton the GASP clips are authored on |

Adding MCO Mocap Basics to the project restores both `Content/MCO_Mocap_Basics/` and the `Source/...` files above; nothing in the C++ module (`Source/ATLA/`) depends on them.

## Epic Game Animation Sample (GASP)

| Content | Source | Path in project | Restore |
|---|---|---|---|
| GASP Blueprints (retarget AnimBP, character, choosers) | Epic Games, Game Animation Sample project (Epic Games Launcher -> Learn / Samples, create it as a 5.8 project) | `Content/Blueprints/` | Copy `Content/Blueprints` from the local sample project into the same path |
| GASP animation library (about 2,345 clips) and the `GQ_M_Neutral_*` locomotion loops | same | `Content/Anims/GASP/` and the sample's own package paths | Copy from the local sample project at matching package paths; `Design/anim-windows.md` documents the integration state |

This is Epic sample content: usable in Unreal projects under the Unreal Engine EULA, not redistributable as source assets.

## Mixamo

| Content | Source | Path in project |
|---|---|---|
| Raw FBX downloads (about 114 clips: kicks, punches, capoeira set, casting gestures, dodges) | mixamo.com (free Adobe account) | `Import/Mixamo/` |
| Imported source skeleton, mesh and clips | derived | `Content/Anims/Mixamo_Src/` |
| Retargeted clips on the game skeleton (`MXQ_` prefix) | derived | `Content/Anims/Mixamo/` |

Mixamo's terms allow use inside a project but not redistribution of the animation files. To rebuild:

1. Re-download the clip list in `Design/anim-windows.md` (current window manifest) and `Design/art-implementation-plan.md` item 0.4 from mixamo.com as **FBX for Unreal, Without Skin, 30 fps**, except `Mma Kick.fbx` which must be downloaded **with skin** (it provides the base skeleton). Save them under `Import/Mixamo/`.
2. With the editor open: `python3 Tools/ue_run.py Tools/import_mixamo.py` (set `MIXAMO_LIMIT=5` for a smoke test).
3. Then `python3 Tools/ue_run.py Tools/build_mixamo_retarget.py` to build the IK rig and retargeter and batch-retarget every clip onto the mannequin.

## CMU Graphics Lab Motion Capture Database (included)

`Import/CMU/*.fbx`, `Import/CMU/converted/*.fbx`, and the derived `Content/Anims/CMU/`, `Content/Anims/CMU_Manny/`, and `Content/Anims/Retarget/` assets **are** tracked. The CMU database is free for any use, including commercial, with the only restriction being resale of the raw data. Files were obtained as pre-converted FBX from the HuggingFace `gbionics/cmu-fbx` mirror; `Import/CMU/MANIFEST.md` maps each clip to its subject and description.

Attribution: "Data obtained from mocap.cs.cmu.edu. The database was created with funding from NSF EIA-0196217."

## Epic template content (included)

`Content/ThirdPerson/`, `Content/Variant_Combat/`, `Content/Variant_Platforming/`, `Content/Variant_SideScrolling/`, `Content/LevelPrototyping/`, `Content/Characters/Mannequins/`, `Content/Characters/UE5_Mannequins/`, `Content/Input/`, and the corresponding `Source/ATLA/Variant_*` C++ come from the Unreal Engine 5.8 Third Person C++ template. They are included under the Unreal Engine EULA content terms, which permit distributing engine template content as part of an Unreal project. The MIT licence in `LICENSE` does not apply to them.

## Not tracked, not third-party

`Import/Reallusion/` is an empty placeholder for an optional paid Tai Chi mocap set that was never purchased.
