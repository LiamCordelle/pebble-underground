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
Pre-commit QEMU-tested PBW SHA-256: `bc708c9d2a0d810d37d1fa8d3d0d0eb9ac08f0f022c4e93fb19e2e16f8cf11ce`.
Release build provenance records the consumer source revision and dependency.
Store publication and verification are recorded in store/published.json.

## Published 1.1.0 — 1 November 2026

Source release commit: `5f93535e2c6b53a152847a091c12ceb81999bd54`.
Published build SHA-256: `04c78f2f9c738b7e06a26d7ace1ca86034b28a63413397d0e029431b36df8e43`.
The committed-source clean build changes SDK resource timestamps and JSON
capability ordering; executable bytes outside the SDK resource-timestamp field,
resources and phone code match the broader eight-zone runtime-tested build.
The publish command uploaded this final artifact without changing its hash.
Authenticated store records confirm the existing app, visible published 1.1.0,
PBW and retained gallery. See `store/published.json` and `store/publish-1.1.0.log`.
Final uploaded-artifact QEMU verification also passes Mumbai (WORLD 289),
Clay save/restart, retained 12,500-step goal, pixel-exact clock glyphs and
clean exits without runtime faults. Its screenshots and report are in
`shots/emulator-publication/`; the emulator is stopped and SDK profile restored.
