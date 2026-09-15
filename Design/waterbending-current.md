# Waterbending — current implementation reference

This is the framework every other element extrapolates from. All systems are C++ (GAS) in `Source/ATLA/Bending/`.

## Resource model
- **Water** attribute (0–100, start 40): the bending resource. No passive regen — refilled by channeling from `ATLAWaterSource` pools in the level (hold RMB, ~40/s, must be within 600cm). *This is water's identity: the element is environmental — you carry what you drew.*
- **Chi** attribute (0–100, +8/s regen): the mobility resource (dodge costs 12). Element-agnostic.

## Technique ladder (cost / cooldown / input)
| Technique | Cost | CD | Input | Behavior |
|---|---|---|---|---|
| Water Whip | 1 | 0.25s | LMB (hold = auto) | Rapid-fire serpentine projectile, 15 dmg. Flow-chain animation ping-pongs strike sections; alternating hands |
| Ice Spears | 10 | 1s | Q | Fan of 5 straight cone darts, 8 dmg each, siblings pass through each other |
| Ice Wall | 25 | 1s | E | 5.2m wall erupts from ground, physically blocks, melts+dies at 8s |
| Octopus Form | 50 | 10s | R | 8 segmented tendrils orbit 10s; auto-lash (10 dmg/0.6s), grab-hoist-swing-throw one victim |
| Draw | — | — | RMB hold | Channel from pool; ends on release/full/out-of-range |
| Dodge / Roll | 12 chi | 0.35s | Shift (x2 = roll) | LaunchCharacter burst; chained window 0.6s |

## Architecture patterns (reuse for all elements)
- `UATLACastAbility` base: montage + `CastDelay` release + `CastDuration` blend-out + `OnCast()` override + `GetMontageStartSection()` for strike variety + crosshair-true `GetCrosshairAimRotation()`. Bending stance (strafe/aim-facing) auto-applied on cast.
- Costs/cooldowns/damage = tiny `UGameplayEffect` subclasses (`ATLAGameplayEffects.cpp`); cooldown tags gate activation, cost check refuses when broke.
- Native gameplay tags per element/technique/cooldown (`ATLAGameplayTags`).
- Upper-body montages (slot `UpperBody`, re-slotted duplicates in `/Game/Bending/Anims/`) play over locomotion; full-body variants only when planted.
- Visuals: engine primitives + generated translucent materials (M_Water/M_Ice pattern in `/Game/Bending/VFX/`), serpentine mesh offset + distance-emitted stretched trail segments, splash actor on impact.
- Actors: projectile (`ATLAWaterProjectile` + subclass per variant), zone/structure (`ATLAIceWall`), persistent form (`ATLAOctopusForm`). All replicated. **Lifespan must use `SetLifeSpan()`, never `InitialLifeSpan` after BeginPlay.**

## Known TODOs
- Whip flow montage doesn't replicate to simulated proxies (multiplayer visual)
- 2-player listen-server test never run
- Co-op combined bending (design hook: `Combo.Window.Open` tag exists)
- Real bending animation assets (current = retimed template melee swings)

**2026-08-14:** every water cast now visibly draws an arcing stream of globules from the nearest pool into the casting hand (cosmetic; the RMB draw economy is unchanged). M_Water rebuilt with a fresnel rim — glassy, edge-bright, reads as liquid.

**2026-08-14:** near a pool the water meter disappears ("bending the source" — the attribute pins at max); the numbered reserve only governs bending away from water.
