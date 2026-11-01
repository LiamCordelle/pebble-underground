'use strict';
var assert=require('assert');
var options=require('../src/pkjs/app-config');
var schema=require('../src/pkjs/kit/config')(options);
var settings=require('../src/pkjs/kit/settings')(options);
var pkg=require('../package.json');
assert.strictEqual(schema[0].defaultValue,pkg.pebble.displayName);
var expected=pkg.name==='pebble-mondrian'?'mondrian-kit-settings-v1':'underground-kit-settings-v1';
assert.strictEqual(options.storageKey,expected);
assert.strictEqual(options.largeLabels,pkg.name==='pebble-mondrian');
var display=schema[1].items;
assert.strictEqual(display.some(function(v){return v.messageKey==='LARGE_LABELS';}),options.largeLabels);
var memory={};memory[expected]=JSON.stringify({TIME_MODE:2,TEMP_UNIT:1,STEP_GOAL:18000});
var store={getItem:function(k){return memory[k]||null;},setItem:function(k,v){memory[k]=v;}};
assert.strictEqual(settings.load(store).STEP_GOAL,18000);
settings.save(store,settings.load(store));
assert.strictEqual(JSON.parse(memory[expected]).TEMP_UNIT,1);
assert.deepStrictEqual(pkg.pebble.messageKeys,['READY','REQUEST_WEATHER','WEATHER_ID','WEATHER',
  'WEATHER_ERROR','TIME_MODE','TEMP_UNIT','STEP_GOAL','WORLD','WEATHER_ENABLED','DEMO_FALLBACK','LARGE_LABELS']);
assert.strictEqual(options.fullTimezones,true);
var displayItems=require('../src/pkjs/kit/config')(options)[1].items;
assert.strictEqual(displayItems.find(function(item){return item.messageKey==='WORLD';}).options.length,597);
assert(displayItems.some(function(item){return item.id==='kit-timezone-search';}));
[0,3,6,268,289,596].forEach(function(zone){
  settings.save(store,Object.assign(settings.load(store),{WORLD:zone}));
  assert.strictEqual(settings.load(store).WORLD,zone);
});
console.log(pkg.pebble.displayName+' consumer checks passed: UI, existing settings and message-key compatibility.');
