# Changelog

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
