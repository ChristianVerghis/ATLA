# Firebending — implemented spec (designed inline 2026-08-13)

Martial basis: **Northern Shaolin** — aggressive, direct, staccato. The damage element: highest numbers, shortest wind-ups, self-generated resource. Fire runs on **chi** (breath and drive, not environment — Iroh: "fire comes from the breath").

| Technique | Input | Chi | CD | Effect |
|---|---|---|---|---|
| Fire Jab | LMB tap | 4 | 0.3s | 18 dmg bolt, 3000cm/s — fastest basic in the game |
| Fire Blast | LMB hold ≥0.6s | 15 | 2.5s | 35 dmg ball + 12 dmg AoE splash (260cm) |
| Fire Arc | Q | 12 | 1.5s | 3-bolt horizontal fan (Azula's sweeping arcs) |
| Wall of Flame | E | 25 | 8s | 8s flame line: burns crossers 10/0.5s, **incinerates enemy projectiles** (Jeong Jeong) |
| Inferno Nova | R | 45 | 12s | 30 dmg + knockback radial burst (Ozai's comet-charged bursts) |
| Fire Jet | F | 15 | 3s | Propulsion dash along aim (downward aim clamped up — always lifts off) + flame kick at the feet; chain casts to fly (Azula) |
| Breath of Fire | RMB hold | — | — | Channel: +12 chi/s surge (slowed movement); breath is the root of fire |

Rhythm: fastest montage rates in the game (jab 2.0×, 0.08s release). Counterplay: fire wall answers water's projectile spam; nova punishes octopus-range aggression; fire out-bursts everyone but starves without breath discipline. Placeholder look: emissive orange translucent (M_Fire), volcanic-field home zone with lava veins and obsidian spires.

**Look (v2, 2026-08-14):** white-hot core bolts with flickering heads and rising ember trails; impacts flare upward (embers rise — never a splash); the flame wall is licking cone tongues with nested white cores and cycling sparks; the nova detonates with a flash column and radial flame gouts. Materials: M_FireCore (emissive 30), M_Fire, M_Ember.

## Rework — 2026-08-14

| Change | Detail |
|---|---|
| Q | **Fire Lash** replaces the 3-bolt arc: one wide burning crescent, 22 dmg, heavy structure damage |
| RMB tap | Toggles **Breath of Fire** on/off (was hold-channel) |
| RMB hold >=1.2s | **Lightning** — Azula's cold-blooded fire: crackling blue-white charge at the fingertips (slowed while charging), release strikes instantly at the crosshair for 50 dmg (35 chi, 10s CD). Early release fizzles free. |
| F | Jet retuned for CHAINING: 0.8s cooldown, 8 chi (was 3s/15) — rhythm-chain jets to fly |
