# Element extrapolation roadmap

Waterbending (see `waterbending-current.md`) is the proven framework. Each element below gets a scheduled design deep-dive producing an implementation-ready spec: `Design/firebending.md`, `Design/earthbending.md`, `Design/airbending.md`.

## What each element spec must define

1. **Resource identity** — water carries drawn water; each element needs its own canonical logic:
   - *Fire*: self-generated (chi-driven? builds with aggression? sun/rage mechanics from lore?)
   - *Earth*: environmental like water, but positional (must stand on/near earth; the ground itself is ammo)
   - *Air*: abundant but defensive-biased (lore: airbenders evade and redirect; scarcity of pure offense is canon)
2. **Technique ladder** — 4–6 moves mapped to the cost tiers (spammable basic ~1, volley ~10, defensive structure ~25, signature form ~50), each with: lore citation (episode/user), damage/cost/cooldown numbers, actor type (projectile / zone / structure / form / mobility), and which `ATLACastAbility` pattern it reuses.
3. **Defence** — every element needs at least one physical defensive option (wall/deflect/zone) that interacts with enemy projectiles.
4. **Mobility move** — element-flavored dodge upgrade or traversal (water spout, earth launch, fire jet, air scooter).
5. **Movement/animation notes** — the martial art each style is based on (fire=Northern Shaolin, earth=Hung Gar, air=Baguazhang, water=Tai Chi), what strike rhythm fits (fire=staccato, earth=rooted/heavy, air=circular/evasive), and which montage/section treatment approximates it with current assets.
6. **Counterplay matrix** — how the element's moves interact with the other elements' walls/projectiles/forms.

## Constraints for all specs
- Must build on existing C++ patterns (CastAbility base, GE cost/cooldown classes, native tags, replicated actors, upper-body montage variants) — no engine features we haven't proven.
- Placeholder visuals: engine primitives + one generated material per element (M_Fire/M_Earth/M_Air following the M_Water/M_Ice python pattern).
- Numbers tuned against water's (basic ≈15 dmg burst DPS, wall 8s, form 10s/10s CD).

## Status
- [x] Firebending spec — `firebending.md`; **IMPLEMENTED 2026-08-13** (7 techniques, volcanic zone, gauntlet-verified)
- [x] Earthbending spec — `earthbending.md`, reviewed & approved 2026-08-06; **IMPLEMENTED 2026-08-06** (full kit + element loadouts + structure damage, gauntlet-verified)
- [x] Airbending spec — `airbending.md`; **IMPLEMENTED 2026-08-13** (6 techniques + passives, sky-temple zone, gauntlet-verified)
- [ ] Cross-element counterplay pass (after all three)

Specs are produced by scheduled cloud research agents (claude.ai/code/routines) and land as run reports; they get reviewed and committed here as `Design/<element>.md` during interactive sessions. Once a spec is committed and approved, implementation follows the checklist inside it.

**Signature empowerment (2026-08-14):** every element's R ultimate grants 10s of Empowered — +50% to all damage dealt, regular projectiles 1.45x bigger. The ultimate is both a form AND a power window; weaving basics inside it is the new optimal rhythm.
