# Changelog

## 1.0.3 — 20 October 2026

- Reconstruct today's heart-rate range from recorded minute BPMs, including
  earlier readings while other faces were active, with bounded asynchronous reads.
- Mark incomplete history with an asterisk; retain the range if the current
  reading is unavailable. Current filtered BPM and transit geometry are unchanged.
- Kit 1.3 combines this history fix with Underground's existing pace formatter.
- Preserve the 1.0.2 Watchfaces-list icon.

## 1.0.2 — 19 October 2026

- Add a native 25x25 red, green and purple transit-route icon to the watch's
  Watchfaces list.
  Regenerate it with `python3 tools/create_menu_icon.py` (requires Pillow).

## 1.0.1 — 19 October 2026

Initial public release for Pebble Time 2.

- Horizontal UV route with band stations, current train and daily-peak ring.
- Separate label lanes for heart rate, steps, UV and the second time zone.
- Rolling rain forecast and next sunrise labels across midnight.
- Seven-row Service status board, shown by touch or wrist flick for eight seconds.
- Live health, reliable cached weather, Clay settings and disruption indicators.
- Kit 1.2, including Mondrian 1.0.4's current-heart-rate availability fix.

Validation: 57 host-rendered scenes, kit regression suite, 143,192 independent
timezone checks and final Emery interaction/settings/weather/cache checks.
Observed QEMU frames: 3–17 ms; clean app exit with no remaining allocations.

1.0.0 was the local design and verification build.

Published listing: https://apps.repebble.com/c977d7b7e07b49298007a427
