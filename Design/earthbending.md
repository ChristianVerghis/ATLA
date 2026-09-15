# Earthbending — implementation-ready design spec

Extrapolated from `waterbending-current.md`. Martial basis: **Hung Gar** (rooted stances, patient waiting, decisive heavy strikes; Toph's close-range variant is Southern Praying Mantis). Doctrine: **neutral jing** — "wait and listen before striking" (Bumi, *Return to Omashu*, S2E3). Earth is the slow, heavy element: fewer, bigger hits; longer wind-ups; the biggest single payoffs in the game.

## 1. Resource identity — positional ammunition

Water carries what it drew; **earth is ammo you stand on**.

- **Earth** attribute (0–100, start 60), added to the existing attribute set alongside Water/Chi.
- **Passive regen +6/s, but only while standing on bendable earth.** Zero regen mid-air or on non-earth surfaces. No pool actors, no draw channel — ground contact *is* the draw.
- **Grounded gate:** every earth ability requires the `State.Earth.Grounded` tag. Mid-air or on ice, activation is refused outright (same refusal path as an unmet cost check). An earthbender who leaves the ground is disarmed — canon (Toph loses to Aang's air-hopping in *The Blind Bandit*, S2E6) and the core balance lever.
- **Grounded check:** a 0.15s-interval component tick does a 150cm downward sphere trace (30cm radius) from the capsule base and reads the hit's physical material. Match ⇒ apply infinite regen GE + grounded tag; miss ⇒ remove both.
- **Bendable earth in the snow/ice/rock arena** (by physical material on landscape layers / static meshes):
  - **Bendable:** `PM_Rock` (cliff faces, boulders, rock field), `PM_Dirt` (gravel/dirt patches), `PM_SnowOverRock` (snow-covered ground — the frozen earth beneath is bent; snow is set dressing).
  - **Not bendable:** `PM_Ice` (the frozen lake surface, all ice structures including enemy ice walls), `PM_Wood`/man-made platforms.
- Consequence: the arena has **home turf**. Water pools sit near the lake; the rock field is earth country. The macro game is positional — exactly the element's identity.
- **Root Stance is the RMB draw-equivalent** (see ladder): hold to plant and regen fast, at the price of immobility. Lore: neutral jing itself; Toph teaching Aang to *stand his ground* against the rolling boulder (*Bitter Work*, S2E9).

Sustain math vs water: whip = 60 DPS at 4 Water/s from a carried tank; Rock Jab = ~49 DPS at ~4.4 Earth/s against +6/s grounded regen — earth sustains indefinitely on home turf but trickles, and bursts (Boulder) genuinely spend the bar.

## 2. Technique ladder

| Technique | Input | Cost | CD | Damage | Tier |
|---|---|---|---|---|---|
| Rock Jab | LMB (hold = auto) | 2 | 0.45s | 22 | basic ~1 |
| Boulder | LMB hold ≥0.6s, release | 18 | 3s | 40 + knockback | mid-tier |
| Shard Volley | Q | 10 | 1.5s | 3 × 14 | volley ~10 |
| Earth Wall (+ launch) | E (re-tap E to launch) | 25 (+5) | 4s | 35 slab | defensive ~25 |
| Earth Armor | R | 50 | 15s | — (12s buff) | signature ~50 |
| Root Stance | RMB hold | — | — | — | draw-equivalent |
| Earth Launch | F | 5 Earth + 14 Chi | 2.5s | — | mobility |

**Rock Jab** — *lore:* the standard rock projectile every Earth Rumble VI fighter opens with (*The Blind Bandit*, S2E6). Fist-sized chunk torn from the ground at the caster's feet, hurled at crosshair. Reuses the `ATLAWaterProjectile` pattern (`ATLARockProjectile`): straight, speed 2600 (slower than whip), 40cm cube mesh, gravel-crumb trail. Cast: `CastDelay 0.18s`, `CastDuration 0.30s`, `MontageRate 0.85`. Heavier than whip in every axis — 22 dmg vs 15, 0.45s vs 0.25s — same DPS band, chunkier rhythm.

**Boulder** — *lore:* The Boulder's namesake move; Bumi hurling building-sized rocks one-armed (*Return to Omashu*, S2E3). Holding LMB past 0.6s converts the jab into a charge (the neutral-jing beat: wait, then strike). On release: 1.2m irregular boulder (`ATLABoulderProjectile`, subclass of `ATLARockProjectile`), speed 2000, 40 dmg, knockback 600 via `LaunchCharacter` on victim, **150 structure damage** (two boulders break an ice wall). Cast: `CastDelay 0.9s` (charge loop), `CastDuration 0.4s`, `MontageRate 0.8`, full-body montage when stationary.

**Shard Volley** — *lore:* the Dai Li's rock gloves fired as projectiles (*Lake Laogai*, S2E17); Toph's stream of small ruts and shards. Three heavy shards in a 12° cone (tighter than ice spears' fan), fired as a fast thud-thud-thud sequence 0.12s apart, 14 dmg each, pass through each other like spear siblings. Reuses the ice-spears multi-spawn ability pattern. Cast: `CastDelay 0.25s`, `CastDuration 0.35s`, `MontageRate 0.9`. Vs water's 5×8/1s: 42 dmg per 1.5s, fewer/fatter projectiles that reward aim over spray.

**Earth Wall** — see §3.

**Earth Armor** — *lore:* Toph's rock armor in *The Crossroads of Destiny* (S2E20); Aang's stone shell against Ozai (*Sozin's Comet, Part 4*, S3E21). Signature form, reusing the `ATLAOctopusForm` attached-form pattern (`ATLAEarthArmor`): 8 stone plates snap to body sockets (no orbit — locked, rigid). For **12s**: **40% damage reduction**, **immunity to knockback and to the octopus grab-throw**, Rock Jab cost drops to 1 (you're wearing ammo), movement −15% speed. Ends on duration or death; 15s cooldown (heavier both ways than octopus's 10s/10s). Cast: `CastDelay 0.6s`, `CastDuration 0.5s`, `MontageRate 0.7`, full-body slam-the-fists-together.

**Root Stance (RMB hold)** — the channel that replaces water's Draw. While held: movement locked, stance planted, **Earth regen +25/s total**, **30% damage reduction**, knockback immunity, and **seismic sense** — enemies in contact with bendable earth within 2000cm are outlined through walls (custom-depth stencil, the cheap UE trick). *Lore:* neutral jing (*Return to Omashu*, S2E3); Toph's vibration-sight learned from the badgermoles (*The Blind Bandit*, S2E6). Ends on release. Channeled-drain/gain ability pattern reused from Draw, inverted to a gain.

## 3. Defence — the earth wall, and how it isn't an ice wall

**Earth Wall (E, 25 Earth, 4s CD)** — *lore:* the wall is *the* earthbending move: Toph walling off the drill (*The Drill*, S2E13), Bumi's ramparts, Dai Li barriers. `ATLAEarthWall` follows the `ATLAIceWall` erupting-structure pattern (`SetLifeSpan`, ground-snapped, replicated) with three differences:

1. **Destructible, not melting:** 300 structure HP and a 12s `SetLifeSpan(12.f)` (vs ice's 8s melt). Whip chips 15/hit — water needs 5s of focused fire or a flank; boulders (150) crack it in two.
2. **Shape:** 4.0m wide × 2.8m tall × 0.6m thick — shorter and wider than the 5.2m ice wall. It's cover you fight around, not a curtain.
3. **Pushable — the wall is also an attack** *(implemented 2026-08-06, revised from the original hurled-slab design per playtest feedback)*: re-tap E while your wall lives (within 1500cm, 35° of crosshair) and the bender SHOVES it — the wall slides along the ground at 1400cm/s, dealing **35 dmg + bulldozing knockback** to every character in its path, delivering **300 structure damage** to any wall it meets, and shattering into debris when it hits something solid (or after 35m). The push is exempt from the raise cooldown — raise-then-shove is the intended combo — and costs 5 Earth. *Lore:* Toph and Bumi shoving raised walls into opponents throughout Book 2. Neutral jing as a mechanic: your defence stores an attack.

**Second defensive option:** Root Stance's 30% DR + knockback immunity (above) is the always-available brace — cheap, instant, but immobile; Earth Armor is the premium mobile version. Earth thus defends by *enduring* where water defends by *interposing*.

## 4. Mobility — Earth Launch

**Earth Launch (F, 5 Earth + 14 Chi, 2.5s CD, requires grounded)** — *lore:* earthbenders riding columns and rock waves (General Fong's soldiers, *The Avatar State*, S2E1; Bumi surfing Omashu's streets, *Sozin's Comet*). A 60cm-radius cylinder column (`ATLAEarthColumn`, `SetLifeSpan(2.f)`) erupts under the caster while `LaunchCharacter(Z=1200, forward=450)` fires — reusing the dodge's launch-burst pattern with the column as readable VFX.

Relationship to the chi dodge: dodge (12 Chi) is the universal horizontal escape; Earth Launch is a *vertical commitment*. It jumps walls, escapes the octopus's lash radius, and closes gaps — but the moment you're airborne **all earth abilities are dead** until you land. It's the one move where the rooted element gambles its identity, which is why it costs slightly more chi than a dodge and carries its own cooldown.

## 5. Animation & rhythm — Hung Gar with retimed melee assets

Water fakes Tai Chi flow with high `MontageRate` and ping-ponged strike sections. Earth fakes Hung Gar the opposite way: **lower rates, longer CastDelays, full-body variants whenever the caster is planted**. Weight comes from timing, not new assets.

| Technique | Montage | Section/rate |
|---|---|---|
| Rock Jab | 3-section combo, upper-body slot | alternate sections 1↔2 via `GetMontageStartSection()`, rate **0.85** |
| Shard Volley | 3-section combo | section 3 (the big finisher swing), rate **0.9** |
| Boulder | slow charged-attack montage | rate **0.8**, hold loops the charge, `CastDelay 0.9s` releases at the apex; full-body when stationary |
| Earth Wall | slow charged-attack montage | rate **1.0**, `CastDelay 0.35s` — a stomp-and-raise read |
| Wall launch | combo section 3 | rate **0.75** — the heaviest single swing in the kit |
| Earth Armor | slow charged-attack montage | rate **0.7**, full-body only (you're planted by definition) |
| Root Stance | none | bending stance tag + planted pose; the stillness *is* the animation |

Rhythm target: water is a drumroll; earth is **rest–rest–BOOM**. Every earth CastDelay ≥ 0.18s, every rate ≤ 1.0, and the two biggest moves (Boulder, wall launch) telegraph for nearly a second — opponents get a dodge window, earth gets the biggest numbers in return.

## 6. Counterplay vs water

- **Boulder / wall-slab vs ice wall:** earth is the siegebreaker — two boulders (150 ea) or one launched wall (300) delete a 200 HP ice wall (§7 adds structure HP to `ATLAIceWall`). Water can no longer turtle indefinitely; it must reposition, which is water's strength anyway.
- **Water vs earth wall:** whip/spears are fully blocked and chip 300 HP too slowly to matter; the correct water answer is flanking or ignoring it — earth committed 25 + a long cast to hold a *place*, water wins by refusing to fight there.
- **Octopus form vs earth:** the grab-throw — octopus's scariest line — **fails against Root Stance and Earth Armor** (knockback/grab immunity), so earth's 50-point signature answers water's. But armor doesn't stop the 10 dmg/0.6s auto-lash chip, and a rooted earthbender is a stationary target for spears: octopus + kiting still beats a passive earthbender.
- **Projectile duels:** water out-rates (whip 60 DPS, 0.25s casts, mobile) and wins extended open-field skirmishes; earth out-trades (22/40 dmg chunks, knockback) and wins any exchange where a Boulder connects. Earth's wind-ups are dodgeable on reaction — landing them requires the neutral-jing read.
- **Terrain is the matchup:** on the frozen lake, earth has zero regen and water has pools — water's home. On the rock field, earth regens everywhere and water burns a finite tank far from refills — earth's home. Neither side can camp, because the opponent simply won't come. **Healthy because:** fights migrate across the arena, both elements keep their canon identity (mobile flow vs rooted endurance), and every hard counter (siege, grab-immunity) answers a specific tool rather than the whole kit.

## 7. Implementation checklist

**Attributes / components**
- Add `Earth` attribute (0–100, start 60) to the existing attribute set next to Water/Chi.
- `UATLAGroundedCheckComponent` (new `UActorComponent`): 0.15s timer, 150cm/30cm downward sphere trace, physical-material whitelist; applies/removes `UGE_EarthRegen` + `State.Earth.Grounded`. Server-authoritative, tag replicated.
- Physical materials: `PM_Rock`, `PM_Dirt`, `PM_SnowOverRock` (bendable); assign on arena landscape layers and rock meshes. `PM_Ice`, `PM_Wood` excluded.

**Abilities (all subclass `UATLACastAbility`; all require `State.Earth.Grounded`)**
- `UATLARockJabAbility` (LMB tap, hold = auto like whip), `UATLABoulderAbility` (LMB hold ≥0.6s release), `UATLAShardVolleyAbility` (Q), `UATLAEarthWallAbility` (E; owns the re-tap launch branch), `UATLAEarthArmorAbility` (R), `UATLAEarthLaunchAbility` (F), `UATLARootStanceAbility` (RMB channel, Draw's channel pattern inverted to gain).

**Actors (replicated; lifespans via `SetLifeSpan()` only)**
- `ATLARockProjectile` : `ATLAWaterProjectile` — speed 2600, 22 dmg, cube mesh + crumb trail.
- `ATLABoulderProjectile` : `ATLARockProjectile` — speed 2000, 40 dmg, knockback 600, 150 structure dmg; scale-3 slab variant (speed 2200, 35 dmg, knockback 800, 300 structure) spawned by wall launch.
- `ATLAEarthWall` : pattern of `ATLAIceWall` — 400×60×280cm, 300 HP, `SetLifeSpan(12.f)`, `Launch()` method.
- `ATLAEarthArmor` : attached-form pattern of `ATLAOctopusForm` — 8 socket-locked cube plates, no orbit.
- `ATLAEarthColumn` — 60cm cylinder, `SetLifeSpan(2.f)`, spawned by launch.
- **Cross-element change:** add 200 structure HP to `ATLAIceWall` (currently time-only) so siege interactions resolve.

**Gameplay tags (native, `ATLAGameplayTags`)**
`Ability.Earth.Jab`, `.Boulder`, `.Volley`, `.Wall`, `.Armor`, `.Launch`, `.Root`; `Cooldown.Earth.Jab` (0.45s), `.Boulder` (3s), `.Volley` (1.5s), `.Wall` (4s), `.Armor` (15s), `.Launch` (2.5s); `State.Earth.Grounded`, `State.Earth.Rooted`, `State.Earth.Armored`.

**Gameplay effects (tiny subclasses in `ATLAGameplayEffects.cpp`)**
- Costs: `UGE_EarthCost_Jab` (2), `_Boulder` (18), `_Volley` (10), `_Wall` (25), `_WallLaunch` (5), `_Armor` (50), `_Launch` (5 Earth; reuse chi-cost class for the 14 Chi).
- Cooldowns: `UGE_EarthCooldown_*` matching the durations above.
- Regen/buffs: `UGE_EarthRegen` (+6 Earth/s, infinite while grounded), `UGE_RootBonus` (+19 Earth/s, 30% incoming-damage multiplier, knockback immunity tag, while channeling), `UGE_ArmorBuff` (12s: 40% incoming-damage multiplier, immunity tags, −15% move speed).
- Damage: `UGE_EarthDamage_Jab` (22), `_Boulder` (40), `_Shard` (14), `_Slab` (35).

**Placeholder visuals (python-generated, `/Game/Bending/VFX/`, M_Water/M_Ice script pattern)**
- `M_Earth` — opaque: BaseColor (0.22, 0.16, 0.10), Roughness 0.92, Metallic 0. Rocks, walls, columns, armor plates.
- `M_EarthLight` — opaque snow-dusted accent: BaseColor (0.45, 0.42, 0.38), Roughness 0.85. Wall top edge, column caps (arena palette blend).
- `M_EarthGlow` — emissive: Emissive (0.15, 0.9, 0.35) × 4.0 intensity — Earth Kingdom crystal green (crystal catacombs, *The Crossroads of Destiny*). Boulder charge-up pulse, armor plate seams, root-stance ground ring.
- Meshes: engine cubes non-uniformly scaled/rotated for all rocks; tapered box for the wall; cylinder for columns; jab trail = distance-emitted crumb cubes reusing the water trail emitter.

---

## Review (2026-08-06) — APPROVED with amendments

1. **Grounded check v1 simplification.** The arena has no landscape layers or physical materials (retinted static meshes). V1 rule: grounded = standing on any walkable surface that is not an `ATLAWaterSource` pool or an ice/earth wall actor. The `PM_*` whitelist in §1 is the v2 refinement, added when the arena gets real surface authoring.
2. **LMB input clarified.** Tap = Rock Jab (no hold-to-autofire, unlike water); hold ≥0.6s = Boulder charge, release to fire. Earth's basic rhythm is deliberate taps — rest–rest–BOOM.
3. **Structure damage plumbing** (missing from checklist): walls have no ASC, so GAS damage can't touch them. Add `StructureHP` float + `ApplyStructureDamage()` to a small `IStructureDamageable` interface on `ATLAIceWall`/`ATLAEarthWall`; projectiles call it in their impact handler with their structure-damage value. This is the cross-element task gating all siege interactions.
4. **Element loadouts are a prerequisite.** Root Stance's RMB collides with water's Draw while the test character holds every ability. Before earth implementation: per-character element kit selection (grant one element's ability set). Seismic sense's through-wall outline: deferred to v2 (renderer work, no 1v1 demo value).

## Amendment — 2026-08-14: Boulder Hoist replaces Root Stance on RMB

Hold RMB: a huge boulder (1.7x the charged boulder) tears out of the ground ahead
with a debris burst and hovers, orbited by loose chips, while the bender braces —
**rooted** (knockback immune) and slowed, the root stance folded into the lift.
Release hurls it at the crosshair (auto-throws after 4s — a boulder is heavy);
release before it tears free and the ground swallows it back. Cost 30 earth, 6s CD.

Also new: every earth cast visibly **rips chunks out of the ground** into the
casting hand (draw stream), and the charged LMB hold shows a growing stone orb
at the hand alongside the wind-up animation.
