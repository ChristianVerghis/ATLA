# Art sourcing — verified findings (researched 2026-08-14)

## VFX (Niagara) — the free stack
- **Niagara Examples Pack** (Epic, free, Fab): 50+ ready systems — fire, smoke, mist, trails, sparks, explosions. Best single grab. fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600
- **Particle Effects** (Epic learning project, free): the arctic-cave level remastered on Niagara — blizzard, waterfalls, flowing pools, magical flames. Doubles as arctic-zone reference. fab.com/listings/e2dfbe93-639e-4690-87d8-4988c9c08ee6
- **M5 VFX Vol2: Fire and Flames** (permanently free, Fab): candles→bonfires→explosions, recolorable. Firebending base. fab.com/listings/c5b0270a-a295-4644-a4be-42cb1e56a197
- **Realistic Fire & Explosion Vol.1 Free Starter** (Fab): 10 systems with exposed User Parameters (scale/intensity/color from C++). fab.com/listings/2adf4e4e-5f1d-4543-9e8c-192fe3c9a8ed
- ~~CGHOW free project files~~ **CORRECTED (round 2): CGHOW files are PAID** ($8–20 each on Gumroad, rest Patreon-gated). His YouTube tutorials remain free — rebuild-by-hand is the free path.
- ~~Niagara Fluids~~ **CORRECTED (round 2): Fluids is broken on Apple Silicon/Metal** (verified M3 Max reports, sims render no color) — use sprite/flipbook fire-smoke instead. The Water plugin itself is fine.
- Gaps: no verified free dedicated water/wind/rock packs — cover with Examples Pack mist/trails + Fluids + Megascans debris meshes.
- **Fab biweekly free drops** (fab.com/limited-time-free): claim-to-keep; current window ends Aug 25. The sourcing routine checks each cycle.
- License: Epic content is UE-only but fine for a UE game; third-party = Fab Standard License.

## Meshes + materials
- **Quixel Megascans**: the free-for-all ended Dec 2024; what remains free: the **1,500-asset starter pack**, **monthly free drops**, and **Megaplants** (2026 vegetation line, all free, Nanite-ready). Paid assets ~$0.99+. Library is strongest exactly on our biomes: Icelandic cliffs, lava fields, snow/ice, quarry slabs.
- Sky temple needs stylized/architectural packs instead (routine hunts these).

## Animations (bending styles → martial arts)
- **Game Animation Sample Project** (Epic, free, **updated for 5.8**): 500+ Motion Matching locomotion clips on the native Manny skeleton. The locomotion foundation; our GAS montages layer on its slots.
- **Waterbending = Tai Chi**: Reallusion **Martial Arts Taichi** — 18 pro mocaps, **$39** (only real Tai Chi mocap set found anywhere).
- **Firebending = Northern Shaolin**: Fab **Motion Cast Kung Fu #06** (~$45, 144 mocap anims by a real practitioner) or **Motifect Martial Arts Pack** (free, 40 anims) to start.
- **Earthbending = Hung Gar**: nearest is **Bajiquan** in Reallusion Martial Arts Vol.1 ($117 for Tai Chi + Drunken Fist + Bajiquan — supersedes the $39 pack if bought).
- **Airbending = Baguazhang**: **no commercial mocap exists** (verified absent) — composite from acrobatic/ninja packs + mage casts, or hand-key/custom mocap later.
- **Mixamo**: still free, maintenance mode; generic kung fu + casting gestures; UE 5.4+ one-click IK retarget (no root bone — fix known).
- Casting gestures: **Caster Animation Pack** (Fab, Epic-skeleton native — zero retarget).
- Suggested budget: **$39 now** (Taichi) or **~$160** for Taichi+Bajiquan+Kung Fu — covers three elements' style identity.

## Weekly cloud routines (repurposed 2026-08-14)
- Tue 9pm ET — **Niagara VFX specs** (element rotation, emitter stacks + C++ integration points)
- Wed 9pm ET — **mesh/material sourcing** (zone rotation + Fab free-drop watch)
- Fri 9pm ET — **animation pipeline** (element rotation: clip mapping, retarget path, montage spec)
Outputs land in the claude.ai routine sessions until the GitHub App is authorized for ChristianVerghis/ATLA.

## Deep-dive round 2 (2026-08-14, verified)

### VFX corrections + new finds
- **Lush "Stylized Fire VFX"** (Fab, FREE — author-verified): Genshin-style anime fire, Niagara presets + recolorable material instances. **The best show-style firebending base found — supersedes M5 as the primary.** fab.com/listings/d2855faa-e4df-4acd-8f08-07dd5933d276
- **VfxSTOCK "Stylish Fire VFX"** (Fab, free): 4+ Niagara fire effects, runtime recolorable. fab.com/listings/01e8534c-5877-4ce2-8948-9a696100de11
- **Free Slash VFX** (Fab, free): anime slash arcs — retint white-blue for air strikes. fab.com/listings/192cfced-5884-4104-98ec-adf39fd2a88f
- **Sparks Embers** (Fab, free): customizable spark/ember collection — volcanic ambience. fab.com/listings/48e20eb7-0812-4670-b206-44d3b5aa8a01
- **M5 Fire and Flames is native Niagara now** (listing retitled "(Niagara)", UE 5.0+) — no Cascade conversion needed.
- **Particle Effects arctic project is still Cascade-era** — don't claim it for systems; use it as arctic *reference* only. The CascadeToNiagaraConverter exists if we ever want its blizzard (P_Blizzard, P_WaterFall).
- **Lightning**: Epic's official Niagara *Static Beam* template + Jitter Position module + ribbon Curve Tension ≈ 0.5 — free, exactly a lightning bolt.
- **Ambience from-scratch recipes** (no free packs needed): camera-attached GPU systems — Spawn Rate → Shape Location box → Curl Noise Force → Drag ± Gravity (negative-light for rising embers) → Camera Offset. Ground fog = large soft depth-fade sprites. Heat shimmer = refraction material on a plane, not particles.

### Fab → project on macOS (verified)
In-editor: Window → **Fab** → sign in → My Library → "+" adds to the open project (plugin enabled by default). Mac fallback if the in-editor button misbehaves (known mac reports): **Epic Games Launcher → Fab Library → Add to Project**. Learning projects (GASP, Content Examples): Launcher → Library → **Create Project**, then Migrate folders into ATLA.

### GASP 5.8 migration recipe (verified)
Migrate `Content/Characters/UEFN_Mannequin/Animations` (incl. MotionMatchingData) and `Content/Blueprints` → ATLA/Content. Enable plugins FIRST or references silently break: **PoseSearch, Chooser, MotionTrajectory, AnimationWarping, AnimationLocomotionLibrary**.

### Animation round 2
- **Airbending = Mixamo capoeira set** (free): ginga loop, Martelo Do Chau, Chapa-Giratoria, Bencao Kick, Au To Role, Esquiva variants, Butterfly Twirl + Cartwheel/Backflip/Front Flip — the closest thing to Baguazhang that exists free.
- **Fire shortlist (Mixamo)**: Mma Kick, Roundhouse/Flying/Hurricane/Jump Spin Kick, Cross/Hook/Elbow Punch, Fist Fight A/B, Leg Sweep.
- **Casting (water-ish until tai chi)**: Standing 2H Magic Attack 01–05, 1H Magic Attack 01–03, Magic Heal — slow, circular, retimable.
- **Dodges/idles**: Standing Dodge Back/L/R, Fight/Warrior/Ninja Idle.
- **More free packs**: MoCap Online Free Animation Pack (fab.com/listings/64c53af0-dcb7-4483-9d65-5cbc84bd9a93), MC Sample Animation Pack — 120+ clips incl. combat + spell-cast (fab.com/listings/fba58a40-dc18-475a-b726-b04345f39697), **Paragon hero packs** (permanently free Epic; thousands of combat/caster anims — Gideon/Aurora fit fire/water casters).
- **CMU mocap**: free incl. commercial (no reselling the raw data). Pre-converted **FBX** exists (HuggingFace `gbionics/cmu-fbx`, Academic Torrents) — skip BVH. Subjects: **#12 tai chi**, **#135 karate kata** (Bassai/Empi/kicks), **#144 punch-kick-block combos**, #85 breakdance, #87–90 cartwheels/flips. Noisy data — feet-sliding cleanup expected.
- **Kyokushin karate mocap dataset** (CC-BY-4.0, commercial OK) — earth/fire kata raw material.
- **Bandai Namco dataset: CC BY-NC-ND — NOT usable** in the game.
- Tai chi verdict stands: **no free source anywhere**; Reallusion Taichi $39 remains the waterbending style buy.
