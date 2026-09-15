# Animation windows — manifest + process

## Why this file exists
Every animation bug so far (frozen clips, invisible bodies, backwards facing,
"stretchy" casts, dead windows) came from one root practice: **wiring clip
windows blind**. This file is the manifest of what plays where and the
process that keeps it correct.

## The pipeline rule (never wire blind)
1. **Retarget health first** — after any retarget, run `Tools/anim_strike_scan.py`
   style checks: pelvis horizontal offset ≤ a few cm at all times, pelvis yaw
   constant, limb speeds nonzero *relative to pelvis*. A retargeter with 0 ops,
   an unmapped chain, or a hand→hand "arm chain" all produce silent garbage.
2. **Metric scan** — find candidate moments with a metric that matches the
   *intent* (forward-extension gain for thrusts, dual-hand rise for lifts,
   foot-speed peaks for kicks). Raw "hand speed" favors swings and glitches.
3. **Pose sheet** — render the candidate frames on the actual character
   (`Tools/pose_sheet.py`) and pick by eye. A metric peak can be a stumble.
4. **Wire with timing math** — `window start = peak_time − CastDelay × MontageRate`
   so the visual impact lands exactly on the gameplay event (bolt spawn).
5. **Gauntlet** — `Tools/element_anim_gauntlet.py` after every change:
   pelvis offset (invisibility), limb travel (does it play), yaw deviation
   (facing snaps). All must pass before hand-testing.

## The root-bone trap (cost days — read this first)
This project runs `ERootMotionMode::IgnoreRootMotion` so gameplay movement stays
code-driven. That means **anything the retargeter writes into the root bone is
silently discarded at runtime.**

The IK Retargeter's **Root Motion op** puts the character's whole height in the
root bone (Mixamo output had root z=95.7, pelvis z=-5). Offline evaluation and
`play_animation` compose root+pelvis and look perfect; the moment the clip plays
through an AnimBP slot the root is dropped, the hips fall to the floor, and the
legs fold into the torso — the "legs disappear" glitch. CMU clips were immune
only because their root is (0,0,0) with the height in the pelvis, which is why
earthbending was the one element that always looked right.

**Rule: disable the Root Motion op when baking clips for this project**, and
after any retarget check `root local ~= (0,0,0)` and `pelvis local z ~= 90`.
Symptom-to-cause shortcut: if a symptom splits cleanly along a group boundary
(one element fine, the rest broken), diff the two pipelines before instrumenting
anything — that comparison found this in minutes after days of probing.

## Retarget health (both libraries, verified)
Three settings make or break every clip; check them after ANY retargeter change:
- **Retarget pose aligned** — `auto_align_all_bones(SOURCE, CHAIN_TO_CHAIN)`.
  Without it the source bind pose's arm roll (CMU: 46 degrees of shoulder yaw)
  is baked into every transferred rotation and the arms wrench behind the back.
- **Pelvis rotation pinned** — `rotation_alpha = 0`, so the body keeps facing
  the crosshair instead of snapping to the performer's capture-room facing.
- **Pelvis translation is axis-split** — `translation_alpha = 1`,
  `scale_vertical = 1`, `scale_horizontal = 0`. The axes want opposite things:
  vertical ON lets the hips drop into crouches and lunges (fully pinning it
  makes the legs over-extend straight through the floor — that's the "legs
  disappear" bug), horizontal OFF stops travelling clips (capoeira spins,
  sweeps) from dragging the character metres off the capsule.

## Cast timing — bending is the pose
Weight is expressed in three numbers per ability: `CastDelay` (wind-up before
the technique releases), `CastDuration` (how long the form owns the body), and
`MovementScaleDuringCast` (0 = rooted until the form finishes, 1 = free).
Mobility abilities (jet, dodge, scooter, launch) always leave movement at 1.

| Weight | Delay | Duration | Movement | Examples |
|---|---|---|---|---|
| Basic | 0.16-0.22 | 0.45-0.62 | 0.55-0.7 | fire jab, air blast, water whip |
| Mid | 0.28-0.40 | 0.75-0.95 | 0.25-0.45 | lash, swipe, spears, blast |
| Heavy | 0.70-0.85 | 1.40-1.70 | 0.0 (rooted) | fire wall, nova, ice wall |

Dynamic montages blend 0.15 in / 0.35 out so forms flow into each other.

## Current window manifest
Mixamo clips live in `/Game/Anims/Mixamo` (MXQ_ prefix, 114 clips);
CMU in `/Game/Anims/CMU_Manny` (MNY_ prefix, 42).

| Ability | Clip | Window(s) | Intent |
|---|---|---|---|
| Fire jab (LMB) | MXQ Cross_Punch | 0.85 | lunge + thrust (+240 fwd impulse) |
| Fire blast (empowered) | MXQ Roundhouse_Kick | 0.55 | kick release |
| Fire lash (Q) | MXQ Leg_Sweep | 0.28 | low horizontal crescent |
| Fire wall (E) | MXQ 2H_Cast_Spell_01 | 1.00 | two-hand overhead raise |
| Fire nova (R) | MXQ 2H_Magic_Area_Attack_01 | 0.55 | radial burst |
| Fire jet glide (F) | MXQ Warrior_Idle | 0.6 frozen | ride stance |
| Water whip (LMB) | MXQ 1H_Magic_Attack_03 | 0.78, 0.18 | flowing push |
| Ice spears (Q) | MNY taichi_full | 50.9, 20.5, 15.5 | tai chi pushes |
| Air blast (LMB) | MXQ 1H_Magic_Attack_02 | 0.47 | quick palm strike |
| Air swipe (Q) | MXQ armada | 0.69 | capoeira spin kick |
| Earth wall (E) | MNY 144_01 | 5.0 | rising double-palm lift |

Earth jab/spikes and the remaining CMU-wired abilities still use pre-scan
windows — re-run the pipeline on any that read wrong.

**Rejected on measurement:** spinning heel kick (chapa_giratoria_2) for the
lash — great sweep, but it yaws the body ~90 degrees off the crosshair. Prefer
moves that keep facing; the gauntlet's yaw check catches these.

## The real long-term fix
CMU is research mocap: performers wander, drift, and never author "one clean
move, in place". Game-authored packs eliminate the entire failure class:
- **Paragon hero packs** (Epic, free) — in-place, forward-facing combat/caster
  moves on Epic skeletons. Claim Gideon + Aurora first (fire/water casters).
- **GASP Motion Matching** (staged already) — locomotion feel.
- **Mixamo** fire-kick/capoeira sets (free), **Motion Cast Kung Fu #06** ($45,
  real Northern Shaolin), **Reallusion Tai Chi** ($39, fixes soft ice spears).
Show-style references per element: fire=Northern Shaolin, water=Tai Chi,
earth=Hung Gar, air=Baguazhang (avatar.fandom.com/wiki/Firebending etc.).

## Motion-matching locomotion (GASP) — integration state

The Game Animation Sample lives at `~/dev/_samples/GameAnimationSample`.
Its `Content/Blueprints` folder is copied into this project (gitignored, same
rule as the other sample packs — re-copy it rather than committing 36MB).
The Mover-plugin variants (`SandboxCharacter_Mover*`, `MovementModes/`) are
deleted on copy: they need the Mover plugin and we use the CharacterMovement
component. All five required plugins are already enabled (PoseSearch, Chooser,
MotionTrajectory, AnimationWarping, AnimationLocomotionLibrary).

How GASP runs motion matching on a non-UEFN skeleton — it already solved our
exact case:

    BP_Quinn (mesh SKM_Quinn_Simple — our mesh)
      └── AnimClass: ABP_GenericRetarget
            └── runs SandboxCharacter_CMC_ABP on the UEFN skeleton
                and retargets the pose live via RTG_UEFN_to_UE5_Mannequin

So the character keeps its own mesh and skeleton; the motion-matching graph
runs underneath and is retargeted every frame.

**Wired (behind `bUseMotionMatching`, currently OFF):**
1. Mesh AnimClass switches to `ABP_GenericRetarget_C` in BeginPlay. DONE.
2. `UCharacterTrajectoryComponent` on the character. DONE — motion matching
   queries against the predicted path; without it there is nothing to match.
3. Mesh component tag set to the retargeter's path. DONE — the retarget graph
   picks its IK retargeter by async-loading `ComponentTags[0]`, and with no tag
   it throws "index 0 from array ComponentTags of length 0" and freezes.
4. `IK_UE5_Mannequin_Retarget` repointed at our `SKM_Quinn_Simple`. DONE — the
   copied rig had lost its mesh reference, so the retarget had no target.

**Blocked — the pawn hand-off.** With the flag on, the graph loads and runs but
holds a static pose AND zeroes movement (verified: motion matching on = vel 0,
off = vel 600). GASP drives locomotion from its own character: the graph reads
gait/stance/movement state over `BPI_SandboxCharacter_Pawn` / `_ABP`, and its
character applies root motion from the matched pose — which collides with this
project's input-driven movement and `IgnoreRootMotion`.

Two ways to finish it:
- **Blueprint child** of `ATLACharacter` that implements the two GASP
  interfaces and forwards our state (gait from velocity, stance, jump). BP
  interfaces can't be implemented from C++, so this needs graph authoring.
- **Port the pieces** — read what the sandbox character feeds the graph and
  reproduce it in C++, keeping input-driven movement and ignoring the graph's
  root motion.

Before either, decide whether motion matching should drive movement (GASP's
model) or only visuals (ours). That choice determines the whole integration.

**Then:** re-slot the bending montages — they play on `DefaultSlot`, which must
still exist in the retarget graph or the casts stop showing. Gauntlet catches it.

Keep the cast-animation gauntlet green through the change — locomotion and
casts share the same mesh, and step 4 is where they collide.
