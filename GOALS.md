# ATLA — Goals

An Avatar: The Last Airbender-inspired multiplayer bending game (UE 5.8, C++, Gameplay Ability System).

## The three pillars (in priority order)

1. **Movement** — fast, fluid, never animation-locked. Dodge/roll, aim-facing combat stance, full-speed casting. Future: element mobility (water spout, air scooter, earth launch, jet propulsion).
2. **Combat: offence & defence** — every element gets a ladder from spammable basic attack to expensive signature technique, with real defensive options (walls, deflects, zones) that physically interact with projectiles.
3. **Authentic bending styles** — each element's techniques, resource model, and rhythm should feel like its show counterpart. Water = drawn from the environment and carried; other elements get their own canonical resource logic. Art polish is explicitly deferred; authenticity of *mechanics and motion* comes first.

## Working model

- **Interactive sessions**: water-focused implementation and iteration in the editor (Claude drives C++/python, user drives feel-testing and editor-only steps).
- **Scheduled sessions**: research + design extrapolation of the waterbending framework into fire, earth, and air — producing implementation-ready design docs in `Design/`.
- Original design hook, still on the roadmap: **co-op combined bending** (two players channel together for amplified techniques).

## Non-goals (for now)

- Final art style, custom character models, marketplace asset passes
- Open-world scale (one arena at a time)
- Shipping/commercial concerns (private demo; IP rename decision deferred)
