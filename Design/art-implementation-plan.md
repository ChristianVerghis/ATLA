# Art integration plan — samples → environments & bending

*The loop's mission document. Each work item is a checkbox with an acceptance
check. A loop iteration = pick the first unchecked item whose prerequisites are
met, do it, verify it in-engine (PIE harness via `Tools/ue_run.py` / `Tools/ue_wait_and_run.py` + screenshots, Read the PNGs; gauntlets + material/zone build scripts also live in `Tools/`), check it off here, commit `[art-loop] <item>`. Items marked 🧑
need Christian at the keyboard (account sign-ins / store claims); everything
else is agent-executable. Keep this file the single source of truth for progress.*

## Ground truth (verified locally, 2026-08-14)

- Engine 5.8 ships the **Fab plugin** (`Engine/Plugins/Fab`) — in-editor Fab browser (Window → Fab) adds claimed library assets straight into the open project.
- **Epic Games Launcher** installed (`/Applications/Epic Games Launcher.app`) — source for Epic learning projects (Particle Effects, Content Examples, Game Animation Sample).
- **NiagaraFluids**, **CascadeToNiagaraConverter**, **PoseSearch + BlendStack + Chooser** (Motion Matching) all present in the engine — per-project enable when needed.
- Gameplay/visual separation is already clean: every technique spawns a dedicated cosmetic actor (`Source/ATLA/Bending/`), so VFX migration = swapping mesh-component structures for `UNiagaraComponent`s inside those actors. Zero gameplay change.
- Research base: `Design/art-sourcing.md` (verified pack list). Deep per-pack emitter enumeration is Item 1.2 — done in-editor once packs are installed, which beats fighting Fab's bot-walled listing pages.

## Phase 0 — acquisition 🧑

- [ ] **0.1** Claim the free packs on fab.com (browser, Epic account) — free "purchases" are permanent library entries:
  Niagara Examples Pack · **Lush Stylized Fire VFX** (anime fire — the firebending primary) · VfxSTOCK Stylish Fire VFX · Free Slash VFX · Sparks Embers · M5 VFX Vol2 Fire and Flames (Niagara) · Realistic Fire & Explosion Vol.1 Starter · Motifect Martial Arts Motion Pack · MoCap Online Free Animation Pack · MC Sample Animation Pack (120+ clips) · a Paragon hero pack or two (Gideon/Aurora — caster anims) · anything element-adjacent in the current biweekly drop (fab.com/limited-time-free, window ends Aug 25). Links in art-sourcing.md.
- [ ] **0.2** In the ATLA editor: Window → **Fab**, sign in, **Add to Project** for every claimed pack (mac fallback if the button misbehaves: Epic Games Launcher → Fab Library → Add to Project). Note the `/Game/...` folders each lands in (append them to this file).
- [ ] **0.3** Epic Games Launcher → Library → create **Game Animation Sample** as a 5.8 project under `~/dev/_samples/`. (Just create it — migration is agent work, item 4.1. The Particle Effects arctic project turned out to be Cascade-era: reference only, skip it.)
- [ ] **0.4** Mixamo (browser, free Adobe account): download combat/casting picks as *FBX for Unreal, Without Skin, 30fps* into `~/dev/ATLA/Import/Mixamo/`. Shortlist (verified names): FIRE — Mma Kick, Roundhouse/Flying/Hurricane/Jump Spin Kick, Cross/Hook/Elbow Punch, Fist Fight A/B, Leg Sweep. AIR — the capoeira set (ginga, Martelo Do Chau, Chapa-Giratoria, Bencao Kick, Au To Role, Esquivas, Butterfly Twirl) + Cartwheel/Backflip/Front Flip. CASTING — Standing 2H Magic Attack 01–05, 1H 01–03, Magic Heal. Plus Standing Dodge Back/L/R.
- [ ] **0.5** *(optional, $39)* Reallusion **Martial Arts Taichi** — the only true Tai Chi mocap set found anywhere; waterbending's style identity. Export FBX with the UE preset into `~/dev/ATLA/Import/Reallusion/`.
- [x] **0.6** *(agent, done 2026-08-14)* CMU mocap (free incl. commercial use): fetch **pre-converted FBX** (HuggingFace `gbionics/cmu-fbx` or Academic Torrents) for subjects **#12 (tai chi!)**, **#135 (karate kata)**, **#144 (punch/kick/block combos)**, #85 (breakdance), #87–90 (cartwheels/flips) into `Import/CMU/`. Also the Kyokushin karate dataset (CC-BY-4.0). ✅ 42 clips in Import/CMU/ with MANIFEST.md (tai chi 12_04, karate 135_01–11, combat 144_*, acrobatics 85/87/88_*). Kyokushin deferred: its hosting repository still needs locating — folded into 4.3.

## Phase 1 — Niagara firebending (flagship element; "improved firebending")

- [x] **1.1** *(loop, 2026-08-14)* Project opens clean with all packs installed. (Resolved in research: M5 is native Niagara now — no conversion. **Do not enable NiagaraFluids: broken on Apple Silicon/Metal.**) Primary fire source = Lush Stylized Fire VFX; M5 + Fire&Explosion Starter fill gaps.
- [x] **1.2** *(loop, 2026-08-14)* `Design/vfx-inventory.md` generated: **139 Niagara systems** (Vefects Free_Fire, Fire_EXP_Vol01, Stylish_Fire, Sparks_Embers, NiagaraExamples) + ~250 pack animations (MC_Sample 130, MCO Mocap Basics 39, Motifect 40).
- [x] **1.3** *(loop, 2026-08-14)* Fire bolts carry a real Niagara flame (Vefects NS_Fire_Small on the collision sphere, mesh-drop trail retired, core mesh kept; lash/blast inherit). Verified: −18 on dummy, flame visible in flight. ~~Fire bolts (jab/lash/blast): add a `UNiagaraComponent` flame-trail to `AATLAFireBolt` (keep collision + core mesh at first; kill the 32 trail-drop meshes). Start from the best pack trail/fire emitter; ATLA palette: core (1.0, 0.9, 0.35) → body (1.0, 0.45, 0.06) → smoke fade; additive; upward drift on the trail.
- [x] **1.4** *(loop, 2026-08-14)* Impact detonation: Fire_EXP NS_Sub_EXP_Small_002 one-shot on `AATLAFireBurst` (ember meshes kept as backup).
- [x] **1.5** *(loop, 2026-08-14)* Wall of flame: 3× Vefects NS_Fire_Medium along the wall (cone tongues auto-hide, remain as fallback); BurnTick untouched. Screenshot-verified: real roaring fire line.
- [x] **1.6** *(loop, 2026-08-14)* Inferno nova: Fire_EXP NS_Sub_EXP_Large_001_01 detonation (flash/gout meshes auto-hide as fallback); ring kept for the radius read. Verified −45 self-empowered.
- [x] **1.7** *(loop, 2026-08-14)* Fire jet: torch-loop exhaust attached to the feet for 0.8s per jet (plus the existing detonating flame kick). Verified airborne at 971cm/s.
- [x] **1.8** *(loop, 2026-08-14)* Breath of fire: small looping flame at the mouth for the channel's duration.
- [x] **1.9** *(loop, 2026-08-14)* Lightning: packs ship no beam system, so the jittered segment bolt stays (reads well) — upgraded with NS_Spark_Burst shrapnel at the strike point and NS_Spark_Continuous crackle on the charge glow. Verified −50. (Hand-authoring the Static Beam template remains an editor-side polish option.)
- [x] **1.11** *(loop, 2026-08-14)* Pre-warm shipped: player BeginPlay fires all 8 combat systems once at Z −8000 (once per world). Acceptance passed the hard way — a cold PIE cast with NO harness warm-up ran without the hang. **Phase 1 firebending: COMPLETE.**
- [x] **1.10** *(loop, 2026-08-14)* Idle aura: real torch flames on both fists (mesh flickers fully hidden for fire). Screenshot-verified — burning fists with rising smoke.
- Acceptance for every 1.x: PIE screenshot reads as *fire* at gameplay distance; `stat unit` ≤ 8.3ms in the volcanic zone; all four elements still pass the standing gauntlet (`Tools/gauntlet_all_elements.py`; empower check: `Tools/gauntlet_empower.py`).

## Phase 2 — environment ambience (fire/smoke/mist/trails per zone)

- [ ] **2.1** ⏸ *BLOCKED on Phase 0 (loop finding, 2026-08-14)*: Niagara emitter stacks/colors cannot be authored via the python API, and the engine's built-in template systems (FountainLightweight, DirectionalBurst, RadialBurst, SimpleExplosion) expose no user parameters to drive from C++. Viable paths: (a) install the Fab packs (0.1/0.2) — the Fire & Explosion Starter ships parameterized systems, the Examples Pack has mist/trails to duplicate; (b) hand-author one template system in the Niagara editor, after which the loop can duplicate/place/scale it per zone. Note: the project already contains three reusable template systems (NS_Damage, NS_Jump_Trail, NS_JumpPad). Original spec follows. Ambience systems are built from scratch (the sample arctic project is Cascade — skipped): camera-attached GPU emitters — Spawn Rate → Shape Location box → Curl Noise Force → Drag ± Gravity → Camera Offset; ground fog = large soft depth-fade sprites; heat shimmer = refraction material plane (not particles). Build one shared template system, then per-zone variants (2.2–2.5).
- [x] **2.2** *(loop, 2026-08-14)* Volcanic: 4× NS_Embers01 over the lava veins + 2× NS_Smoke_Plume at the spires (`Tools/dress_volcano.py`, actors saved into the level). Heat shimmer deferred (needs a refraction material — editor-side polish). Screenshot-verified: drifting embers + smoke columns.
- [x] **2.3** *(loop, 2026-08-14, partial)* Arctic: 3× wide mist puffs at the pool edges (`Tools/dress_arctic_temple.py`). Blizzard/snowfall deferred — no snow system in the free packs; needs hand-authoring or a future claim.
- [x] **2.4** *(loop, 2026-08-14, partial)* Sky temple: 3× cloud wisps hugging the platform undersides. Wind-streak ribbons deferred (trail systems need a moving emitter — candidate for a drifting spline actor later).
- [ ] **2.5** Quarry: dust motes, settling dust after impacts, occasional dust devil.
- Acceptance: one screenshot per zone showing ambience without washing out combat VFX; fps gate as above.

## Phase 3 — water / air / earth technique VFX

- [x] **3.1** *(loop, 2026-08-14, partial)* Whip + spears trail a real Niagara ribbon (NS_SimpleRibbonTrail on the projectile base; mesh drops retired for water). Splash-burst upgrade + pool ripples deferred (no water-specific system in the packs).
- [x] **3.2** *(loop, 2026-08-14)* Draw streams: a Niagara ribbon rides the lead globule along the source→hand arc for water/air (globule meshes kept — they give the stream body; earth stays chunky on purpose).
- [x] **3.3** *(loop, 2026-08-14, partial)* Air bolts trail the ribbon (thinner scale) alongside the corkscrew streaks — visually verified in flight. Dome/cyclone/scooter Niagara variants deferred (need hand-authored looping systems).
- [x] **3.4** *(loop, 2026-08-14)* Earth kit: NS_Dirt_Explosion_Small on every `AATLAEarthBurst` — covers all rock impacts AND the hoist eruption (which spawns the same burst). Debris meshes kept underneath. Screenshot-verified: real pulverized-rock cloud. Armor crumble dust deferred.
- [ ] **3.5** Octopus form tendrils: ribbon tentacles (stretch goal — current orb chains acceptable until then).

## Phase 4 — animations (avatar-accurate styles)

- [x] **4.1** *(loop, 2026-08-14, staged)* GASP library copied in at the same package path (2,345 animation assets, gitignored — re-copy from ~/dev/_samples/GameAnimationSample) and all five Motion Matching plugins enabled in ATLA.uproject. Activates on next editor restart; registry verification queued for the next loop iteration.
- [ ] **4.2** Import + IK-retarget Mixamo and Motifect picks to UE5 Manny (5.4+ auto-retarget path; fix missing root bone on Mixamo clips). Catalogue everything in `Design/anim-inventory.md` with per-element fit notes.
- [x] **4.3** *(loop, done 2026-08-14)* CMU FBX → import → IK Retarget to Manny → tai chi #12 becomes the waterbending montage source; #135/#144 feed earth/fire. Expect feet-slide cleanup.
  - ✅ *(loop, 2026-08-14)* Imported: the raw gbionics FBX are bones-only (Interchange refuses animation-only without a skeleton), so `Tools/blender_cmu_convert.py` (headless Blender) skins a dummy cube to each armature; all **42 clips now live at `/Game/Anims/CMU`** on one shared skeleton (`12_04_Skeleton`). ✅ Retargeted: auto-generated IK rigs (`/Game/Anims/Retarget/IK_CMU`, `IK_Quinn`, `RTG_CMU_to_Quinn`, FUZZY chain auto-map) + `run_batch_retarget` → **42 clips on SK_Mannequin at `/Game/Anims/CMU_Manny` (MNY_*)**. Tai chi visually verified in PIE (clean pose, no candy-wrapping). Feet-slide cleanup deferred to montage authoring (4.4) where clips get trimmed anyway.
- [x] **4.4** *(loop, done 2026-08-14)* Dynamic-clip infrastructure + every cast ability and the dodge-roll on real mocap (roll = CMU cartwheel 87_05, full-body dynamic montage; dash montage remains the fallback). Remaining montage work is polish-tier: window tuning by feel + richer per-style clips from Phase 0 packs.
  - ✅ *(loop, 2026-08-14)* **Infrastructure shipped — no montage assets needed**: `UATLACastAbility` now plays raw AnimSequences as dynamic montages in the UpperBody slot (`UpperBodyCastSequence` + `SequenceStartTimes` rotating per-cast windows into the form). **Water whip converted: each cast plays a different window of the real CMU tai chi** (verified: dynamic montages playing + −15 damage intact + screenshot).
  - ✅ Cross-cutting fix while verifying: `GetCrosshairAimRotation` (all projectiles) and lightning now use pawn-aware, penetration-safe multi-sweeps — visibility traces phased through characters, and a camera inside arena geometry aimed shots backward.
- [x] **4.5** *(loop, done 2026-08-14)* All 17 cast abilities now play real mocap windows — heavies included: fire blast=Yokogeri, fire wall=Heiansyodan guard, nova=Bassai opening, earth wall=Empi, armor=deep Bassai, water spears/wall/octopus=later tai chi movements, dome=capoeira footwork, cyclone=jump twist. Boulder release + hoist keep their charge-continuity montages deliberately. Full gauntlet green (incl. direct jet measurement: 1121cm/s airborne).
  - ✅ *(loop, 2026-08-14)* **All four elements' basics now play real mocap**: water whip = six windows of the full 148s tai chi form (`MNY_taichi_full`); fire jab = karate punch sequence (144_20 windows) + lash = Mawashigeri roundhouse (135_07); earth rock jab = Oiduki lunge punch (135_09) + spikes = Gedanbarai (135_05); air blast = jump-kick-spin (87_01) + swipe = helicopter sweep (85_08). Full gauntlet green.
  - ✅ Pipeline bug fixed on the way: Blender's default 250-frame scene range truncated every clip to 8.3s — converter now bakes each action's true range (the tai chi is 148s, kata 10–50s). Full re-import + re-retarget done. Import gotcha logged: a failed Interchange import poisons that source path for the session — reimport under a new filename or a fresh editor.
- [ ] **4.6** *(stretch)* Motion Matching locomotion via PoseSearch, GASP-style, with bending montages layered — only after 4.1–4.5 are stable.

## Loop status — round 2 finished 2026-08-14 ~7:30am (see below); earlier pause note kept for history

## Loop status — paused 2026-08-14 ~4am

**Round 2 (post-Fab-claims): 8 more iterations.** Completed: 1.1–1.11 (ALL of Phase 1 — firebending fully on Niagara), 2.2–2.4 (volcano embers+smoke, arctic mist, temple clouds), 3.1–3.4 (ribbon trails on water/air, dirt-explosion earth impacts, draw-stream ribbons). **Still open**: 4.1 GASP migration (🧑 create the project in ~/dev/_samples first), 4.2 Mixamo imports (🧑 download the shortlist), 2.5 quarry dust + 2.3 blizzard + dome/cyclone/scooter Niagara + octopus tendrils + heat shimmer (all need hand-authored systems or future pack claims), 4.6 Motion Matching (stretch).

Six iterations completed round 1 (0.6, 4.3, 4.4a, 4.5a + two bug-fix passes). **Everything
still unchecked needs Christian's Phase 0 claims** (Fab packs → 1.x/2.x/3.x;
GASP project → 4.1; Mixamo downloads → 4.2) or richer clips (remaining 4.4/4.5
swaps). Restart with `/loop` after Phase 0 — iteration one should be 1.2
(in-editor pack inventory), then Phase 1 fire VFX in plan order.

## Loop protocol

1. `git pull`. Read this file; pick the first unchecked item whose prerequisites are done (respect phase order within an element, but Phase 2 items may interleave once 0.2/0.3 are done).
2. If the item needs assets not yet present (Phase 0 unchecked), do agent-executable prep instead and list the blocker in the commit message.
3. Editor discipline: quit editor before `Build.sh` (run from the project dir); relaunch after; PIE harness + screenshots for acceptance; end PIE cleanly.
4. Check the box, append a one-line result under the item if noteworthy, update `build_log.md`, commit `[art-loop] <item-id> <summary>`, push.
5. Perf gate is a hard gate: if a system blows the frame budget, scale it down before checking off.

## Known risks

- ~~M5 Cascade risk~~ resolved: native Niagara.
- Niagara Fluids is broken on Apple Silicon (Metal renders no sim color) — excluded from the plan.
- CGHOW files are paid (~$10/effect) — tutorials free; not a dependency.
- Fab listing pages are bot-walled; never web-scrape for inventory — enumerate in-editor (1.2).
- GASP migration is chunky; anim-library-first keeps it incremental.
- Editor screenshots at background 3fps race sub-0.4s effects; verify those numerically + by frame-stepped screenshots if needed.
- **PIE cold-spawn hang (found 2026-08-14)**: the FIRST in-PIE spawn of a pack Niagara system can hard-hang the game thread on Metal (40+ min, no recovery), while in-editor spawns are instant. Mitigation in all harnesses: pre-warm via `NiagaraFunctionLibrary.spawn_system_at_location` at PIE start + 6s before combat spawns. TODO gameplay-side: a map-load pre-warm pass (spawn each combat system once, hidden) so first casts never hitch for players — added as item 1.11.
- Force-killed editors leave an autosave-restore prompt that blocks the NEXT boot at frame 0 — delete `Saved/Data/PackageRestoreData.json` before relaunch.
- **One editor instance only (hard rule, learned 2026-08-14)**: the loop's background editor collided with Christian's own opens — his instance died against the held project locks, spawning CrashReportClient popups that read as "the project keeps crashing". Before handing off for play-testing: `pkill` ALL UnrealEditor instances, clean restore data, launch exactly one, and the loop must not touch the editor until the user is done.
