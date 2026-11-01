# Underground kit integration

## 1.1.0 timezone upgrade — 1 November 2026

Updated the clean shared-kit dependency from kit 2.2.0
`c737afcfadf8cdd62dec58941cf8718fed0d6017` to kit 2.4.1
`2dcfa7302bf07aabb9a1f5f0974064b638dd5ac8` after fetching origin/main.
The selected revision is the published, tested main used by Mondrian 1.1.0.
No shared-kit edits were required. This face opts into `fullTimezones: true`;
the kit owns search, transport, validation, persistence and offline transitions.
Existing storage namespace, message keys and valid saved goals are preserved.

Display → Find a time zone filters by city, region or IANA name; explicitly
select a result and save. Empty search shows common choices plus the selected
zone. Search text is temporary. The bundled tzdata 2026f covers 2020–January
2038; future timezone rule changes arrive in watchface updates.

The face owns the preview WORLD override and render scenarios for Mumbai,
Abu Dhabi, Moscow, Kathmandu, Chatham, Kiritimati and UTC−12 in both time formats.
Release receipts in build/ record the final kit SHA and PBW hash.

Validation completed for 1.1.0: shared/native and consumer tests passed;
`npm run release -- --render` passed 91 scenes. QEMU saved and restarted
eight timezone selections through Clay, retained a 12,500-step saved goal,
and matched the live clock glyphs against deterministic renders in both time
formats. Fresh defaults remained 8,000. All nine app exits freed allocations;
no runtime faults were reported. The owned emulator was stopped and the
original SDK profile restored. Screenshots and reports are in
`shots/emulator-timezones/`. Default prototype artwork still passes its pixel checks.
PBW SHA-256: `bc708c9d2a0d810d37d1fa8d3d0d0eb9ac08f0f022c4e93fb19e2e16f8cf11ce`.
Release build provenance records the consumer source revision and dependency.
Store publication and verification are recorded in store/published.json.
