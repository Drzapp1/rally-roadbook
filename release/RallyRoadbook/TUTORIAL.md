# How to read a rally roadbook

A roadbook replaces a GPS line. Instead of "follow the magenta arrow," you get a
scrolling strip of little diagrams (**tulips**) plus distances and compass
headings, and you match them to the terrain as you ride. This guide teaches the
skill the plugin is built to train.

## 1. The anatomy of a case

Each row ("case") on the rally computer has three columns:

```
┌──────────┬───────────────┬──────────┐
│  12,34   │      ╱         │    R     │   ← turn abbreviation (R / L / kpS…)
│  (TOTAL) │   tulip + ↑    │   088    │   ← CAP (compass heading out, °)
│ ┌─────┐  │   (the shape  │   !!     │   ← danger level
│ │0,30 │ 7│    you ride)   │  [sign]  │   ← terrain / hazard pictograms
│ └─────┘  │               │          │
└──────────┴───────────────┴──────────┘
   distance       diagram       what to do
```

- **TOTAL** (big, top-left) — kilometres from the start. Your trip meter's TOTAL
  must read this when you reach the junction.
- **Partial** (boxed, bottom-left) — metres since the *previous* case. This is the
  number you count down to the next instruction.
- **The case number** sits beside the partial.

## 2. Reading a tulip

The tulip is a bird's-eye schematic of the junction, **always drawn travel-up**:

- **You always enter from the bottom.** The small **dot** is where you come in.
- The **line** is the road/track shape through the junction.
- The **arrowhead** is the way **out** — the direction you leave.
- A faint **cross-stub** means another track crosses here (don't take it).

So a tulip that goes straight up then bends right with the arrow pointing
top-right means: *carry on, then bear right.* The abbreviation (**R**) and the
**CAP** (e.g. 088°) confirm it.

> Tulips are schematic, not to scale — read the **shape and exit**, not the exact
> angle. Distance comes from the numbers, not the drawing.

## 3. The trip meter (your other instrument)

The twin LCDs show **TOTAL** and **SPEED**. The skill is *distance discipline*:

1. At a case, note the **partial** (say `0,30` = 300 m).
2. Ride, watching the partial count up (or the **NEXT** countdown fall).
3. As it nears the figure, **look up** — the junction the tulip describes should
   be appearing. Act on it.
4. Reset partial at each case (auto, or **F9** to force).

If the terrain doesn't match the tulip at the expected distance, you're probably
off-route — stop and re-read.

## 4. CAP — the compass heading

**CAP** is the compass bearing you should be travelling **as you leave** the case.
The compass tape (top of screen) shows your live heading; line it up with the CAP
and you're pointed right. CAP is your lifeline in featureless desert where the
tulip alone is ambiguous.

## 5. Signs

Pictograms warn of what's there: **danger** `!` / `!!` / `!!!` (rising severity),
terrain (**bump**, **dip**, **water**, **bumpy**, **dune**…), man-made
(**bridge**, **gate**, **railroad**…) and services. Treat `!!!` as *brake now*.

## 6. A training progression (F4 menu)

Build the skill in stages — each removes a crutch:

1. **Full** — everything visible. Learn what the symbols mean.
2. **No-CAP** — hide headings; navigate by tulip + distance alone.
3. **Distance-only** — hide tulips; ride purely on the trip meter and notes.
4. **Masked** — only the current case shown; no reading ahead.
5. **Challenge** — the route is hidden and you're scored (% on-line, off-route
   hits, time). Check **Run history** for your best, and race your **ghost**.

## 7. Tips

- **Read one ahead.** Glance at the next case before you reach the current one.
- **Trust the distance.** When unsure, ride the partial and the junction appears.
- **Recalibrate at obvious features** (a building, a bridge, a fork).
- **Off-route alert** fires when you leave the line — follow the arrow back, then
  re-find your distance.
- Make your own roadbooks: ride a track, **F4 → Save roadbook now**, then study it.
