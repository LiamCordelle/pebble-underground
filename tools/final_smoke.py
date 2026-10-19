#!/usr/bin/env python3
"""Targeted check after the final goal-caption spacing adjustment."""
import os
from pathlib import Path
import shutil
import subprocess
import time
ROOT=Path(__file__).resolve().parents[1]
S=ROOT/'shots'
def run(*args):
    subprocess.run(args,cwd=ROOT,check=True,timeout=60)
def cli(*args):run('pebble',*args,'--emulator','emery')
env=os.environ.copy();env['PYTHONUNBUFFERED']='1'
with (S/'final-smoke.log').open('w') as output:
    log=subprocess.Popen(['pebble','logs','--emulator','emery'],cwd=ROOT,
                         stdout=output,stderr=subprocess.STDOUT,env=env)
    try:
        time.sleep(20)
        cli('emu-button','click','back')
        cli('install')
        sdk=Path(shutil.which('pebble')).read_text().splitlines()[0][2:]
        run(sdk,'tools/launch.py')
        time.sleep(1)
        cli('screenshot','--no-open',str(S/'final-emulator-face.png'))
        cli('emu-tap')
        cli('screenshot','--no-open',str(S/'final-emulator-detail.png'))
        cli('emu-tap')
        cli('emu-button','click','back')
        time.sleep(1)
    finally:
        log.terminate()
        try:log.wait(timeout=5)
        except subprocess.TimeoutExpired:log.kill();log.wait()
        run('pebble','kill')
