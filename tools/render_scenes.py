#!/usr/bin/env python3
"""Deterministic Emery coverage with the lab renderer, never QEMU."""
import concurrent.futures
import json
import os
from pathlib import Path
import subprocess
from PIL import Image, ImageChops, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
LAB=Path(os.environ.get('WATCHFACE_LAB',ROOT.parent/'watchface-lab'))
SHOTS=ROOT/'shots'
scenes=[('demo',0,False,{}),('stress',1,False,{}),('empty',2,False,{}),
        ('detail',0,True,{}),('detail-stress',1,True,{}),('detail-empty',2,True,{}),
        ('12h',0,False,{'12H':1,'HOUR':9,'MINUTE':5}),
        ('12h-detail',0,True,{'12H':1,'HOUR':9,'MINUTE':5}),
        ('no-sun-detail',0,True,{'NO_SUN':1}),
        ('midnight-sunrise',0,False,{'HOUR':0,'MINUTE':0,'MIDNIGHT_SUNRISE':1}),
        ('midnight',0,False,{'HOUR':0,'MINUTE':8}),
        ('after-sunset',0,False,{'HOUR':21,'MINUTE':15}),
        ('next-sunrise',0,True,{'HOUR':21,'MINUTE':15}),
        ('no-phone',0,False,{'PHONE':0}),('no-health',0,False,{'NO_HEALTH':1}),
        ('no-health-detail',0,True,{'NO_HEALTH':1}),
        ('low-battery',0,False,{'BATTERY':28}),('critical-battery',0,False,{'BATTERY':5}),
        ('charging',0,False,{'BATTERY':62,'CHARGING':1}),
        ('charging-detail',0,True,{'BATTERY':62,'CHARGING':1}),
        ('fahrenheit',1,True,{'FAHRENHEIT':1}),
        ('no-heart-history',0,True,{'HR_MIN':0,'HR_MAX':0}),
        ('partial-heart',0,True,{'HR_PARTIAL':1}),
        ('partial-heart-wide',0,True,{'HR_PARTIAL':1,'HR':220,'HR_MIN':100,'HR_MAX':220}),
        ('partial-no-range',0,True,{'HR_PARTIAL':1,'HR_MIN':0,'HR_MAX':0}),
        ('history-no-current',0,True,{'NO_CURRENT_HR':1}),
        ('partial-history-no-current',0,True,{'NO_CURRENT_HR':1,'HR_PARTIAL':1}),
        ('heart-220',0,False,{'HR':220}),('step-sliver',0,False,{'STEPS':100}),
        ('near-goal',0,False,{'STEPS':9870}),
        ('large-goal',0,False,{'STEPS':49999,'STEP_GOAL':50000,'HR':148}),
        ('no-usual',0,True,{'USUAL_STEPS':0}),
        ('behind-usual',0,True,{'STEPS':3000}),
        ('rain-after-midnight',0,False,{'HOUR':23,'RAIN_MIDNIGHT':1}),
        ('rain-after-midnight-board',0,True,{'HOUR':23,'RAIN_MIDNIGHT':1}),
        ('short-cache',0,False,{'HOUR':23,'RAIN_MIDNIGHT':1,'ROLLING_COUNT':2}),
        ('sunrise',0,False,{'HOUR':7,'MINUTE':29}),
        ('sunset',0,False,{'HOUR':18,'MINUTE':3})]
for uv in (0,20,25,30,50,55,60,70,75,80,100,105,110,160):
    scenes.append(('uv-'+str(uv),0,False,{'UV':uv,'UV_MAX':max(uv,60)}))
    if uv in (25,55,75,105):
        scenes.append(('uv-board-'+str(uv),0,True,{'UV':uv,'UV_MAX':max(uv,60)}))
for name,code in [('sun',0),('partly',2),('cloud',3),('rain',63),('snow',73),('storm',95),('fog',45)]:
    scenes.append(('weather-'+name,0,True,{'WEATHER_CODE':code,'DRY':1}))

def render(scene):
    name,scenario,detail,values=scene
    env=os.environ.copy()
    for key,value in values.items(): env['UNDERGROUND_'+key]=str(value)
    args=['python3',str(LAB/'tools/render.py'),str(ROOT),str(SHOTS/(name+'.png')),'--scenario',str(scenario)]
    if detail: args.append('--detail')
    result=subprocess.run(args,env=env,capture_output=True,text=True)
    if result.returncode: raise RuntimeError(result.stdout+result.stderr)
    return name

with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    list(pool.map(render,scenes))
for name in ['demo','stress','empty','detail','detail-stress','detail-empty']:
    subprocess.run(['python3',str(LAB/'tools/glance.py'),str(SHOTS/(name+'.png'))],check=True,capture_output=True)

# Only the requested exception chip/rain changes may affect the original bowl.
reference=Image.open(LAB/'designs/44-underground2/shots/demo.png').convert('RGB')
actual=Image.open(SHOTS/'demo.png').convert('RGB')
assert ImageChops.difference(reference.crop((0,0,200,119)),actual.crop((0,0,200,119))).getbbox() is None
cols=6;rows=(len(scenes)+cols-1)//cols
sheet=Image.new('RGB',(cols*216,rows*254),'#dedede');draw=ImageDraw.Draw(sheet)
for i,(name,*_) in enumerate(scenes):
    x=(i%cols)*216+8;y=(i//cols)*254
    draw.text((x,y+4),name,fill='black')
    sheet.paste(Image.open(SHOTS/(name+'.png')).convert('RGB'),(x,y+22))
sheet.save(SHOTS/'coverage.png')
(SHOTS/'coverage.json').write_text(json.dumps({'scenes':[s[0] for s in scenes],
    'demo_bowl_pixel_identical':True},indent=2)+'\n')
print(f'Rendered {len(scenes)} scenes; original demo bowl matches pixel for pixel.')
