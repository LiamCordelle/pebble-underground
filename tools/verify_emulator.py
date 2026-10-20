#!/usr/bin/env python3
"""Assert final QEMU interaction/status/cache evidence against host renders."""
from pathlib import Path
import re
from PIL import Image,ImageChops
ROOT=Path(__file__).resolve().parents[1]
S=ROOT/'shots'
def crop(name,box=None):
    im=Image.open(S/name).convert('RGB')
    return im.crop(box) if box else im
def same(a,b): return ImageChops.difference(a,b).getbbox() is None
for name in ('emulator-timeout.png','emulator-toggle.png'):
    assert same(crop('emulator-live.png'),crop(name)),name
assert same(crop('emulator-critical-battery.png',(68,73,132,88)),
            crop('critical-battery.png',(68,73,132,88)))
assert same(crop('emulator-disconnected.png',(68,73,132,88)),
            crop('no-phone.png',(68,73,132,88)))
assert same(crop('emulator-original-live.png',(0,119,200,207)),
            crop('emulator-restarted.png',(0,119,200,207))),'Cached map changed after restart'
logs='\n'.join((S/name).read_text() for name in ('emulator.log','emulator-data.log'))
assert re.search(r'Watchface kit [0-9.]+ started',logs)
assert 'Weather snapshot accepted (48 hours)' in logs
assert 'Kit settings applied: time=1 unit=1' in logs
assert 'Still allocated <0B>' in logs
assert not re.search(r'APP_FAULT|stack overflow|JS failed|app crash',logs,re.I)
frames=[int(n) for n in re.findall(r'Frame \w+ (\d+)ms',logs)]
print(f'Emulator passed: tap timeout/toggle, critical battery, phone loss, settings, '
      f'weather transport/cache/restart, clean exit. Draws {min(frames)}–{max(frames)} ms.')
Image.open(S/'emulator-final.png').save(ROOT/'screenshot_emery.png')
(S/'emulator-results.txt').write_text(f'All assertions passed. Frame times {min(frames)}–{max(frames)} ms.\n')
