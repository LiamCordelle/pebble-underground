"""Check the actual route mapping at a sunrise exactly at midnight."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
text=(root/'src/c/design.c').read_text()
start=text.index('static int day_arc(')
end=text.index('// Index in the chronological rolling forecast',start)
harness='''#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
typedef struct {bool known;int len,s_rise,s_set,rise,set;} DayMap;
'''+text[start:end]+'''
int main(void) {
 DayMap map={true,1000,200,800,0,1080};
 assert(day_arc(&map,0)==200);
 assert(day_arc(&map,1080)==800);
 map.rise=360;assert(day_arc(&map,360)==200);
 map.known=false;assert(day_arc(&map,720)==500);
}
'''
with tempfile.TemporaryDirectory() as temp:
 source=Path(temp)/'day.c';source.write_text(harness);exe=Path(temp)/'day'
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(source),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Route daylight mapping passed: midnight sunrise, sunset and unknown-daylight fallback.')
