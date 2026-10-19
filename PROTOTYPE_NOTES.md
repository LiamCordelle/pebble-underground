# 44 · Underground v2

## Pitch

Your day as a Tube diagram, rebuilt so every value is printed once and reads at a glance.
The Day line is a bowl that cradles the time, switching from night to day service at the
sunrise and sunset interchanges. Under it, an arch carries two lines that meet at an
interchange: the Heart line climbs the left leg with a train at your bpm, and the Steps
line runs over the top to the goal terminus with the count riding beside its train.

## What changed from v1 (28 · Underground)

- **Steps shown once.** The big `6,842 steps` in the middle of the arch is gone. The
  count is now the steps train's label: under the corridor, or beside the right leg as the
  train descends. The 2k–8k station labels became plain station ticks; only the goal
  terminus keeps its `10k`.
- **Heart line added** (spec suggestion): the arch's left leg, plum, 40 bpm at the foot
  to 190 at the interchange. Zone stations at 100 and 140 bpm match the board's statuses.
  The bpm rides beside the train with a heart pictogram.
- **UV added** as a status chip in the middle of the arch, in the board's language: navy
  below 3, brown-orange (the "Minor delays" colour) from 3, dark red (the "Severe" colour)
  from 6, grey when unknown.
- **NZ moved onto the face**, in the arch's bottom row, with a Tube-style airport
  pictogram: NZ is a flight away, the way the map marks Heathrow.
- **Kept:** the bowl and arch, the mitred line rasteriser, night/day colour change at the
  sun interchanges, the rain station, Bitham 42 bold time, and the Service status board.
- **Moved to the board:** temperature and condition, battery and phone.
- **Paper** changed from white to warm cream (`FFFFAA`), Liam's stated preference. The
  board uses the same paper.

## Metric → element

| Metric | Element |
|---|---|
| Time | Bitham 42 bold, centred in the bowl |
| Time of day | Day-line train, time-proportional within night, day and night sections (v1) |
| Date | Gothic 24 bold under the time |
| Sunrise / sunset | Interchange rings at the two bends, where the line changes colour; times above them in Gothic 18 bold |
| Rain peak | Rain station (tick, drop, hour) at the rest of today's peak hour if it reaches 30%. It sits just above the line and slides sideways past the rings, never across a line |
| Steps | Steps train position, ticks every fifth of the goal, and the count beside the train (Gothic 24 bold). Goal terminus ring, filled on arrival; `10k` beside it until the count needs the room |
| Heart rate | Heart train on the left leg, bpm beside it with a heart (Gothic 24 bold) |
| UV | Status chip in the middle of the arch (Gothic 24 bold, white on the chip colour) |
| NZ time | `✈ NZ 22:09`, Gothic 18 bold, bottom row |
| Everything else | The Service status board |

Missing data: with no weather the Day line runs red and evenly timed, with no rings, sun
times or rain station, and the chip reads `UV --` in grey. With no heart rate the heart
train is hidden and the label reads `♥ --`. With no step data the count reads `--`.

## Detail view: Service status board

The board Liam loves, kept and made more readable. A navy header with the time, then six
rows: a coloured line box, a status, and one line of detail. Disruptions sit in a chip
(brown-orange for minor, dark red for severe).

| Line | Statuses | Detail |
|---|---|---|
| Day | Good service in daylight, Night service otherwise | **New:** countdown to the next sun event (`Sunset in 7h54`), since both sun times are on the face |
| Steps | Good service / Minor delays / Severe delays against your usual pace, Goal reached, Not yet running | **New:** `1,722 ahead · due 12:03`. The face has the count, so the board gives the comparisons. "Due" is when the train reaches the goal terminus at today's pace, using Summit's ETA formula |
| Heart | Good service, Running fast (100+), Express service (140+), No information | bpm and today's range |
| Weather | Good service, Minor delays (30%+ rain later), Severe delays (storm, snow, 70%+ rain now) | rain % and hour, temperature |
| Battery | Good service, Reduced service (<40%), Part suspended (<10%), Planned works (charging) | % |
| Phone | Good service, Suspended | Connected / Disconnected |

Statuses moved up from Gothic 14 bold to 18 bold. Details moved from Gothic 14 regular to
14 bold, and the line boxes shrank slightly so the longest detail fits without an
ellipsis.

## Readability

Checked with `tools/glance.py` every round (`shots/demo_glance.png`,
`shots/detail_glance.png`, plus `stress_glance.png`).

- **Enlarged:** the date from Gothic 14 bold to Gothic 24 bold. Sun times, rain hour,
  `10k` and NZ went from Gothic 14 bold to 18 bold. Heart rate went from Gothic 14 bold to
  Gothic 24 bold, with an 11 px heart. Board statuses went from 14 to 18 bold. In v1's
  glance, all of these were mush.
- **Added at must-have size:** UV (Gothic 24 bold in a chip) and the bpm, both readable in
  the glance image.
- **Dropped:** the 2k/4k/6k/8k station labels (the count beside the train does the job),
  the `steps` caption, and the temperature, weather glyph and battery row (all to the
  board).
- **Moved:** the step count from the arch centre to beside the steps train (each metric
  once).
- Result: in the glance image every must-have (time, date, steps, heart rate, UV) and every
  station label reads. The board's statuses read; its Gothic 14 bold details are marginal
  at glance size but fine on a deliberate tap.

## Palette

| Role | Code | Corrected |
|---|---|---|
| Paper (face and board) | `FFFFAA` | `#fff6d3` |
| Ink, night line, header band | `000055` | `#001e41` |
| Day line, Day box | `FF0000` | `#e35462` |
| Steps line, Steps box | `00AA55` | `#5c9b72` |
| Heart line, Heart box | `AA0055` | `#983e5a` |
| Rings, trains | `000000` / `FFFFFF` | black / white |
| UV 3–5 chip, Minor delays chip | `AA5500` | `#9d5b4d` |
| UV 6+ chip, Severe delays chip | `AA0000` | `#99353f` |
| UV unknown chip, Phone box | `555555` | `#545454` |
| Weather box | `0055AA` | `#16638d` |
| Battery box (navy text) | `FFAA00` | `#f1aa86` |
| Board rules | `AAAAAA` | `#ababab` |

## Fonts

- Time: system `BITHAM_42_BOLD` (30 px ink, starts at box y + 12). Its round geometric
  figures feel closest to transport lettering.
- Values (date, steps, bpm, UV): `GOTHIC_24_BOLD`, 14 px digits at box y + 10.
- Station labels, NZ, board statuses and header: `GOTHIC_18_BOLD`, 11 px digits at box
  y + 7.
- Board details and line names: `GOTHIC_14_BOLD`.
- No resources added. The drop, heart and plane are 1-bit string-row pictograms in
  `design.c`.

## Emery rendering lessons

- **Labels must know about lines, not just other labels.** v1's slide-to-free-spot search
  could push a label down onto the line it labels. v2's rain label slides sideways along
  its own tier (toward the middle), then tries the sun tier, and never moves vertically by
  the corridor. Labels beside a vertical leg slide only within that leg's span.
- **Descenders count.** The date's keep-out box needs about 4 px more than its digit
  height, or a station label tucks under the `p` of `Sep`.
- **A small plane must be diagonal.** A side-on plane at 9 px reads as an arrow-plus-cross.
  The diagonal ✈ silhouette reads as a plane at 1x.
- **Probe metrics before laying out.** One render of test strings plus an ink-bbox script
  gave exact ink offsets and widths for every font in about a minute. `lab_text_size`
  widths: Gothic 14 bold `Sunset 18:03 · NZ 22:09` = 129 px, `Weather` = 52.
- **Test beyond the three scenarios.** A throwaway harness (`scratch/cases.py`) injected
  six edge states into `draw()`: trains on every segment, HR at both ends, night, noon, the
  12h clock and rain near each ring. It found the label-on-line bug the fixed scenarios
  never hit.
- AA stays off for the whole face: lines and rings are pixel-exact, and chip corners are
  stepped, not blended.

## Ideas worth stealing

1. **One arch, two lines.** Join two metrics end to end at an interchange: two gauges in
   one shape, and the colour change explains itself.
2. **Count as the train's label.** Print a value once, beside the marker that already
   shows it, instead of as a separate readout.
3. **Board chips for live values.** The UV chip reuses the status board's disruption
   colours, so "UV 7" in dark red reads as "severe" without a legend.
4. **"Due" as an ETA.** Transit vocabulary makes the Summit ETA instantly understood:
   "due 12:03".

## Credits

- Mitred line rasteriser, arc-length engine, trains, rings, rain-peak logic and the
  Service status board: from `designs/28-underground` (v1).
- Goal ETA: Summit's `eta_minute` formula (`designs/21-summit`), copied as required so
  every face reports the same ETA.

## Known limitations and next steps

- The UV chip shows `uv_x10` as given (current UV in Signal Deck). With live data, the
  daily maximum might be more useful on a face you glance at in the morning.
- When the rain station sits on a night arm next to a sun ring, it takes that sun time's
  slot. This happens in stress, where the sunset has already passed. This is v1's rule.
- Near sunrise or sunset the day train sits on a 45° bend over the ring (a train at the
  station). It's accurate, but it's the lumpiest moment of the day.
- With 3-digit bpm in the 120–160 range and a 2-digit UV, the heart label and chip are
  about 8 px apart (the stress shot): clear, but the tightest spot on the face.
- The heart zone ticks are subtle at 1x. They're texture; the bpm is the readout.
- Battery and Bluetooth live only on the board. A small disruption marker on the face for
  critical battery or a lost phone may be worth adding if Liam misses them.
- Checked with `render.py` only (pixel-identical to the emulator), not on a real watch.
