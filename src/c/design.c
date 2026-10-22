#include <string.h>
#include "lab.h"
#include "kit/kit_core.h"

// 44 · Underground v2 (BRIEF.md section 14.44), from 28 · Underground.
//
// Your day as a Tube diagram. The Day line is a bowl that cradles the time. Under
// it, an arch carries two lines that meet at an interchange: the Heart line climbs
// the left leg (zone stations, the train at your bpm) and the Steps line runs over
// the top and down the right leg to the goal terminus. Each value is printed once,
// beside its train.

// Palette ----------------------------------------------------------------------
#define C_PAPER GColorFromHEX(0xFFFFAA)
#define C_WHITE GColorWhite
#define C_INK GColorFromHEX(0x000055)
#define C_DAY GColorFromHEX(0xFF0000)
#define C_NIGHT GColorFromHEX(0x000055)
#define C_STEPS GColorFromHEX(0x00AA55)
#define C_UV GColorFromHEX(0x5500AA)
#define C_HEART GColorFromHEX(0xAA0055)
#define C_RING GColorBlack
#define C_ALERT GColorFromHEX(0xAA0000)
#define C_WARN GColorFromHEX(0xAA5500)
#define C_TRAIN GColorBlack

typedef struct {
  int16_t x, y;
} Pt;

static GFont s_time_font;  // Bitham 42 bold
static GFont s_value;      // Gothic 24 bold: date, steps, bpm, UV
static GFont s_label;      // Gothic 18 bold: station labels, board
static GFont s_small;      // Gothic 14 bold: board details

static void load(void) {
  s_time_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  s_value = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_label = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_small = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
}

static int sgn(int v) {
  return (v > 0) - (v < 0);
}

static int iabs(int v) {
  return v < 0 ? -v : v;
}

// Route engine -------------------------------------------------------------------
// A route is a polyline of horizontal, vertical and 45-degree segments between
// integer vertices (line centres sit on pixel centres). It's rasterised without
// anti-aliasing as the union of mitred bands, so 45-degree runs are perfect
// 1-px staircases and every bend is a clean mitre. (From 28 · Underground.)

static void unit_dir(Pt a, Pt b, int *ux, int *uy) {
  int dx = sgn(b.x - a.x), dy = sgn(b.y - a.y);
  int k = (dx != 0 && dy != 0) ? 707 : 1000;
  *ux = dx * k;
  *uy = dy * k;
}

static void fill_run(GContext *ctx, int x0, int x1, int y) {
  if (x1 >= x0) {
    graphics_fill_rect(ctx, GRect(x0, y, x1 - x0 + 1, 1), 0, GCornerNone);
  }
}

// Segment i of pts[0..n-1] as a mitred band. hw = half width of straight runs,
// hd = half run of diagonal rows (hw 2 / hd 3 gives 5 px lines).
static void draw_segment(GContext *ctx, const Pt *pts, int n, int i, int hw, int hd) {
  Pt a = pts[i], b = pts[i + 1];
  int ux, uy;
  unit_dir(a, b, &ux, &uy);
  int sx = ux, sy = uy, ex = ux, ey = uy;
  if (i > 0) {
    int px, py;
    unit_dir(pts[i - 1], a, &px, &py);
    sx += px;
    sy += py;
  }
  if (i + 2 < n) {
    int nx, ny;
    unit_dir(b, pts[i + 2], &nx, &ny);
    ex += nx;
    ey += ny;
  }
  int dx = sgn(b.x - a.x), dy = sgn(b.y - a.y);
  bool diag = dx != 0 && dy != 0;
  int pad = hd + 2;
  int x_lo = (a.x < b.x ? a.x : b.x) - pad, x_hi = (a.x > b.x ? a.x : b.x) + pad;
  int y_lo = (a.y < b.y ? a.y : b.y) - pad, y_hi = (a.y > b.y ? a.y : b.y) + pad;
  for (int y = y_lo; y <= y_hi; y++) {
    int run0 = 0, run1 = -1;
    for (int x = x_lo; x <= x_hi + 1; x++) {
      bool inside = false;
      if (x <= x_hi) {
        int rx = x - a.x, ry = y - a.y;
        bool band;
        if (diag) {
          band = iabs(rx * dy - ry * dx) <= hd;
        } else if (dy == 0) {
          band = iabs(ry) <= hw;
        } else {
          band = iabs(rx) <= hw;
        }
        inside = band && (rx * sx + ry * sy >= 0) &&
                 ((x - b.x) * ex + (y - b.y) * ey <= 0);
      }
      if (inside) {
        if (run1 < run0) {
          run0 = x;
        }
        run1 = x;
      } else if (run1 >= run0) {
        fill_run(ctx, run0, run1, y);
        run0 = 0;
        run1 = -1;
      }
    }
  }
}

static void draw_poly(GContext *ctx, const Pt *pts, int n, int hw, int hd, GColor color) {
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_fill_color(ctx, color);
  for (int i = 0; i + 1 < n; i++) {
    draw_segment(ctx, pts, n, i, hw, hd);
  }
}

// Arc length in 1/100 px (diagonal steps count 141).
static int seg_len(Pt a, Pt b) {
  int dx = iabs(b.x - a.x), dy = iabs(b.y - a.y);
  return (dx && dy) ? dx * 141 : (dx + dy) * 100;
}

static int arc_to(const Pt *pts, int vertex) {
  int s = 0;
  for (int i = 0; i < vertex; i++) {
    s += seg_len(pts[i], pts[i + 1]);
  }
  return s;
}

// Point at arc length s; also returns the segment index.
static Pt route_point(const Pt *pts, int n, int s, int *seg) {
  for (int i = 0; i + 1 < n; i++) {
    int len = seg_len(pts[i], pts[i + 1]);
    if (s <= len || i + 2 == n) {
      s = lab_clamp(s, 0, len);
      int dx = sgn(pts[i + 1].x - pts[i].x), dy = sgn(pts[i + 1].y - pts[i].y);
      int unit = (dx && dy) ? 141 : 100;
      int k = (s + unit / 2) / unit;
      if (seg) {
        *seg = i;
      }
      return (Pt){pts[i].x + dx * k, pts[i].y + dy * k};
    }
    s -= len;
  }
  if (seg) {
    *seg = n - 2;
  }
  return pts[n - 1];
}

// The part of a route between arc lengths s0..s1.
static int sub_route(const Pt *pts, int n, int s0, int s1, Pt *out) {
  int seg0, seg1;
  Pt p0 = route_point(pts, n, s0, &seg0);
  Pt p1 = route_point(pts, n, s1, &seg1);
  int m = 0;
  out[m++] = p0;
  for (int i = seg0 + 1; i <= seg1; i++) {
    if (pts[i].x != out[m - 1].x || pts[i].y != out[m - 1].y) {
      out[m++] = pts[i];
    }
  }
  if (p1.x != out[m - 1].x || p1.y != out[m - 1].y) {
    out[m++] = p1;
  }
  return m;
}

static void draw_section(GContext *ctx, const Pt *pts, int n, int s0, int s1, GColor c) {
  if (s1 <= s0) {
    return;
  }
  Pt sub[10];
  int m = sub_route(pts, n, s0, s1, sub);
  if (m >= 2) {
    draw_poly(ctx, sub, m, 2, 3, c);
  }
}

// Stations ----------------------------------------------------------------------

static void draw_ring(GContext *ctx, Pt p, int r) {
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_fill_color(ctx, C_RING);
  graphics_fill_circle(ctx, GPoint(p.x, p.y), r);
  graphics_context_set_fill_color(ctx, C_WHITE);
  graphics_fill_circle(ctx, GPoint(p.x, p.y), r - 2);
}

// Tick on a straight run, pointing to side (nx, ny).
static void draw_tick(GContext *ctx, Pt p, int nx, int ny, GColor c) {
  graphics_context_set_fill_color(ctx, c);
  if (ny != 0) {
    int y = ny < 0 ? p.y - 2 - 3 : p.y + 3;
    graphics_fill_rect(ctx, GRect(p.x - 1, y, 3, 3), 0, GCornerNone);
  } else {
    int x = nx < 0 ? p.x - 2 - 3 : p.x + 3;
    graphics_fill_rect(ctx, GRect(x, p.y - 1, 3, 3), 0, GCornerNone);
  }
}

// Terminus: a bar across the end of a vertical line.
static void draw_buffer(GContext *ctx, Pt p, GColor c) {
  graphics_context_set_fill_color(ctx, c);
  graphics_fill_rect(ctx, GRect(p.x - 5, p.y - 1, 11, 3), 0, GCornerNone);
}

// Train: a short, wide section of track centred on arc length s, with a halo
// and a row of windows. It snaps to lie wholly on one straight segment (a train
// bent round a mitred corner turns into a lump). Returns its centre.
static Pt draw_train(GContext *ctx, const Pt *pts, int n, int s, int *seg_out) {
  const int half = 800;
  int seg;
  route_point(pts, n, s, &seg);
  int a = arc_to(pts, seg);
  int b = a + seg_len(pts[seg], pts[seg + 1]);
  s = (b - a >= 2 * half) ? lab_clamp(s, a + half, b - half) : (a + b) / 2;
  int s0 = s - half, s1 = s + half;
  Pt sub[10];
  int m = sub_route(pts, n, s0, s1, sub);
  draw_poly(ctx, sub, m, 5, 7, C_PAPER);
  draw_poly(ctx, sub, m, 4, 6, C_TRAIN);
  graphics_context_set_fill_color(ctx, C_WHITE);
  for (int w = s0 + 350; w <= s1 - 350; w += 400) {
    Pt p = route_point(pts, n, w, NULL);
    graphics_fill_rect(ctx, GRect(p.x - 1, p.y - 1, 2, 2), 0, GCornerNone);
  }
  if (seg_out) {
    *seg_out = seg;
  }
  return route_point(pts, n, s, NULL);
}

// Pictograms ------------------------------------------------------------------------
// One family: 1-bit, drawn on an integer grid from string rows.

static void blit(GContext *ctx, const char *const *rows, int h, int x, int y, GColor c) {
  graphics_context_set_fill_color(ctx, c);
  for (int r = 0; r < h; r++) {
    const char *row = rows[r];
    int run = -1;
    for (int i = 0;; i++) {
      bool on = row[i] == '#';
      if (on && run < 0) {
        run = i;
      } else if (!on && run >= 0) {
        graphics_fill_rect(ctx, GRect(x + run, y + r, i - run, 1), 0, GCornerNone);
        run = -1;
      }
      if (!row[i]) {
        break;
      }
    }
  }
}

static const char *const ICON_DROP[] = {
  "...#...",
  "...#...",
  "..###..",
  ".#####.",
  "#######",
  "#######",
  "#######",
  ".#####.",
  "..###..",
};

// 11 x 10, sized to Gothic 24 bold digits.
static const char *const ICON_HEART[] = {
  ".###...###.",
  "#####.#####",
  "###########",
  "###########",
  "###########",
  ".#########.",
  "..#######..",
  "...#####...",
  "....###....",
  ".....#.....",
};

// Airport connection, as the Tube map marks Heathrow: NZ is a flight away.
static const char *const ICON_PLANE[] = {
  "........##.",
  ".......###.",
  "##....####.",
  ".###.####..",
  "..######...",
  "...####....",
  "..#####....",
  ".##.###....",
  "##..##.....",
  "....##.....",
  "....#......",
};

// Labels --------------------------------------------------------------------------

typedef struct {
  GRect r[20];
  int n;
} Taken;

static bool hits(const Taken *t, GRect a) {
  for (int i = 0; i < t->n; i++) {
    GRect b = t->r[i];
    if (a.origin.x < b.origin.x + b.size.w && b.origin.x < a.origin.x + a.size.w &&
        a.origin.y < b.origin.y + b.size.h && b.origin.y < a.origin.y + a.size.h) {
      return true;
    }
  }
  return false;
}

// Registers an ink box, padded so neighbouring labels keep a clear gap.
static void take(Taken *t, GRect r) {
  if (t->n < (int)ARRAY_LENGTH(t->r)) {
    t->r[t->n++] = GRect(r.origin.x - 4, r.origin.y - 3, r.size.w + 8, r.size.h + 6);
  }
}

// Ink metrics: Gothic 18 bold digits sit at box_y + 7 and are 11 px tall;
// Gothic 24 bold at box_y + 10, 14 px tall.
#define G18_TOP 7
#define G18_H 11
#define G24_TOP 10
#define G24_H 14

// Ink box of a string whose ink starts at (x, ink_y) for the given alignment.
static GRect ink_box(const char *s, GFont f, int x, int ink_y, int h, GTextAlignment align) {
  int w = lab_text_size(s, f).w;
  int bx = align == GTextAlignmentLeft ? x : align == GTextAlignmentRight ? x - w : x - w / 2;
  return GRect(bx, ink_y, w, h);
}

static void put(GContext *ctx, const char *s, GFont f, GRect ink, int top, GColor c) {
  lab_text(ctx, s, f, GRect(ink.origin.x - 2, ink.origin.y - top, ink.size.w + 4, 40),
           GTextAlignmentCenter, c);
}

static void label(GContext *ctx, const char *s, int x, int ink_y, GTextAlignment align) {
  put(ctx, s, s_label, ink_box(s, s_label, x, ink_y, G18_H, align), G18_TOP, C_INK);
}

// Slides r vertically to the free spot nearest its current y within [y_lo, y_hi].
// Returns false if there is none.
static bool find_slot(const Taken *t, GRect *r, int y_lo, int y_hi) {
  int y0 = lab_clamp(r->origin.y, y_lo, y_hi);
  for (int dy = 0; dy <= y_hi - y_lo; dy++) {
    for (int sign = -1; sign <= 1; sign += 2) {
      int y = y0 + sign * dy;
      if (y >= y_lo && y <= y_hi) {
        r->origin.y = y;
        if (!hits(t, *r)) {
          return true;
        }
      }
    }
  }
  return false;
}

// "2k" for whole thousands, otherwise lab_fmt_k ("2.5k").
static void fmt_k(char *buf, size_t n, int32_t v) {
  if (v >= 1000 && v % 1000 == 0) {
    snprintf(buf, n, "%ldk", (long)(v / 1000));
  } else {
    lab_fmt_k(buf, n, v);
  }
}

// Main face ----------------------------------------------------------------------

#define XL 12
#define XR 188
#define BEND 24
#define YD 108          // day line corridor
#define YS 119          // arch corridor
#define Y_TOP 10        // day termini
#define Y_BOT 216       // arch feet
#define Y_JCT (YS + BEND)

static const Pt DAY_PTS[] = {
  {XL, Y_TOP}, {XL, YD - BEND}, {XL + BEND, YD}, {XR - BEND, YD}, {XR, YD - BEND}, {XR, Y_TOP}
};
// The arch: the Heart line is the left leg (arc 0 .. S_JCT), the Steps line the
// rest, from the interchange over the top to the goal terminus.
static const Pt ARCH_PTS[] = {
  {XL, Y_BOT}, {XL, Y_JCT}, {XL + BEND, YS}, {XR - BEND, YS}, {XR, Y_JCT}, {XR, Y_BOT}
};
#define DAY_N ((int)ARRAY_LENGTH(DAY_PTS))
#define ARCH_N ((int)ARRAY_LENGTH(ARCH_PTS))
#define S_JCT ((Y_BOT - Y_JCT) * 100)

#define HR_LO 40        // bpm at the foot of the Heart line
#define HR_HI 190       // bpm at the interchange

typedef struct {
  bool known;
  int len, s_rise, s_set, rise, set;
} DayMap;

// Day line arc positions: night runs from the 00:00 terminus to the sunrise
// interchange (midway down the left bend), day from there to the sunset
// interchange (midway up the right bend), night again to the 24:00 terminus.
// Without sun times the line is one daytime colour and time runs evenly.
static DayMap day_map(const LabData *d) {
  DayMap m;
  m.len = arc_to(DAY_PTS, DAY_N - 1);
  int half_bend = BEND / 2 * 141;
  m.s_rise = arc_to(DAY_PTS, 1) + half_bend;
  m.s_set = arc_to(DAY_PTS, 3) + half_bend;
  m.known = d->weather_ok && d->sunrise_min > 0 && d->sunset_min > d->sunrise_min &&
            d->sunset_min < 1440;
  m.rise = m.known ? d->sunrise_min : 6 * 60;
  m.set = m.known ? d->sunset_min : 18 * 60;
  return m;
}

static int day_arc(const DayMap *m, int minute) {
  if (!m->known) {
    return (int)((int64_t)m->len * minute / 1440);
  }
  if (minute <= m->rise) {
    return m->s_rise * minute / (m->rise > 0 ? m->rise : 1);
  }
  if (minute <= m->set) {
    return m->s_rise + (m->s_set - m->s_rise) * (minute - m->rise) / (m->set - m->rise);
  }
  return m->s_set + (m->len - m->s_set) * (minute - m->set) / (1440 - m->set);
}

// Index in the chronological rolling forecast; never wrap missing hours.
static int rain_peak_hour(const LabData *d) {
  if (!d->weather_ok) return -1;
  int best = -1, best_pct = 29;
  for (int i = 1; i < d->rolling_count; i++) {
    if (d->rolling_rain[i] > best_pct) {
      best_pct = d->rolling_rain[i]; best = i;
    }
  }
  return best;
}

// Header: the time, then the date.
static void draw_header(GContext *ctx, const LabData *d, Taken *taken) {
  char buf[8];
  lab_fmt_time(buf, sizeof(buf), d->now.tm_hour, d->now.tm_min, d->is_24h);
  lab_text(ctx, buf, s_time_font, GRect(20, 2, 160, 50), GTextAlignmentCenter, C_INK);
  take(taken, GRect(44, 14, 112, 30));

  char date[24];
  snprintf(date, sizeof(date), "%s %d %s", lab_day_abbr(d->now.tm_wday), d->now.tm_mday,
           lab_month_abbr(d->now.tm_mon));
  GRect r = ink_box(date, s_value, 100, 52, G24_H, GTextAlignmentCenter);
  put(ctx, date, s_value, r, G24_TOP, C_INK);
  r.size.h += 4;
  take(taken, r);
}

// Ink top of the sun times, 5 px above their rings.
#define SUN_Y (YD - BEND / 2 - 6 - 5 - G18_H)

// Rain station: a tick where today's peak rain chance falls, labelled with a drop
// and the hour. Labels never cross a line: by the corridor and bends they sit just
// above it and slide sideways (toward the middle) past the rings, then try the sun
// tier; by an arm they sit inside the bowl and slide up and down the arm.
static void draw_rain_station(GContext *ctx, const LabData *d, const DayMap *m, int h,
                              Taken *taken) {
  int seg;
  int minute = d->rolling_hour[h] * 60;
  Pt p = route_point(DAY_PTS, DAY_N, day_arc(m, minute), &seg);
  Pt a = DAY_PTS[seg], b = DAY_PTS[seg + 1];
  char t[12];
  lab_fmt_minute_of_day(t, sizeof(t), minute, d->is_24h);
  if (d->rolling_day[h]) strncat(t, "+1", sizeof(t) - strlen(t) - 1);
  int w = 7 + 3 + lab_text_size(t, s_label).w;
  int dir = p.x < 100 ? 1 : -1;
  GRect r = GRect(0, 0, w, G18_H);
  bool ok = false;
  if (a.x == b.x) {
    draw_tick(ctx, p, dir, 0, C_NIGHT);
    r.origin.x = dir > 0 ? p.x + 8 : p.x - 8 - w;
    r.origin.y = p.y - G18_H / 2;
    ok = find_slot(taken, &r, Y_TOP + 4, YD - BEND - G18_H);
  } else {
    if (a.y == b.y) {
      draw_tick(ctx, p, 0, -1, (minute >= m->rise && minute <= m->set) ? C_DAY : C_NIGHT);
    }
    int x0 = a.y == b.y ? p.x - w / 2 : dir > 0 ? p.x + 8 : p.x - 8 - w;
    for (int tier = 0; tier < 2 && !ok; tier++) {
      r.origin.y = tier ? SUN_Y : YD - 2 - 3 - 4 - G18_H;
      for (int dx = 0; dx <= 60 && !ok; dx += 2) {
        r.origin.x = lab_clamp(x0 + dir * dx, XL + 8, XR - 8 - w);
        ok = !hits(taken, r);
      }
    }
  }
  if (ok) {
    blit(ctx, ICON_DROP, 9, r.origin.x, r.origin.y + 1, C_INK);
    label(ctx, t, r.origin.x + 10, r.origin.y, GTextAlignmentLeft);
    take(taken, r);
  }
}

// Day stations: sunrise and sunset interchanges with their times above, then
// the rain station.
static void draw_day_stations(GContext *ctx, const LabData *d, const DayMap *m,
                              Taken *taken) {
  if (!m->known) {
    return;
  }
  Pt p_rise = route_point(DAY_PTS, DAY_N, m->s_rise, NULL);
  Pt p_set = route_point(DAY_PTS, DAY_N, m->s_set, NULL);
  draw_ring(ctx, p_rise, 6);
  draw_ring(ctx, p_set, 6);
  take(taken, GRect(p_rise.x - 6, p_rise.y - 6, 13, 13));
  take(taken, GRect(p_set.x - 6, p_set.y - 6, 13, 13));

  int rain_h = rain_peak_hour(d);
  if (rain_h >= 0) {
    draw_rain_station(ctx, d, m, rain_h, taken);
  }
  // Sun times above their rings, unless the rain station needed the room.
  char t[8];
  int y = SUN_Y;
  for (int i = 0; i < 2; i++) {
    bool tomorrow=!i && d->minute_of_day>=m->set;
    int minute=i?m->set:tomorrow?d->tomorrow_sunrise_min:m->rise;
    if(minute>0) {
      lab_fmt_minute_of_day(t,sizeof(t),minute,d->is_24h);
      if(tomorrow)strncat(t,"+1",sizeof(t)-strlen(t)-1);
    } else snprintf(t,sizeof(t),"--:--");
    GRect r = ink_box(t, s_label, i ? XR - 7 : XL + 7, y, G18_H,
                      i ? GTextAlignmentRight : GTextAlignmentLeft);
    if (!hits(taken, r)) {
      put(ctx, t, s_label, r, G18_TOP, C_INK);
      take(taken, r);
    }
  }
}

// UV joins the Heart and Steps legs. The five station positions mark the
// WHO band starts 0/3/6/8/11; scale continues to 12 and clamps above that.
#define UV_Y 180
static const Pt UV_PTS[] = {{XL, UV_Y}, {XR, UV_Y}};
static int uv_arc(int value_x10) { return lab_clamp(value_x10,0,120) * (XR-XL)*100/120; }
static bool is_night(const LabData *d) {
  return d->weather_ok && d->sunrise_min > 0 &&
    (d->minute_of_day < d->sunrise_min || d->minute_of_day >= d->sunset_min);
}
static void draw_uv(GContext *ctx, const LabData *d, Taken *taken) {
  draw_poly(ctx,UV_PTS,2,2,3,C_UV);
  draw_ring(ctx,UV_PTS[0],6);draw_ring(ctx,UV_PTS[1],6);
  const int bands[] = {0,30,60,80,110};
  for (int i=1;i<5;i++) {
    Pt p=route_point(UV_PTS,2,uv_arc(bands[i]),NULL);
    draw_tick(ctx,p,0,1,C_UV);
  }
  if (d->weather_ok) {
    Pt peak=route_point(UV_PTS,2,uv_arc(d->uv_max_x10),NULL);
    draw_ring(ctx,peak,4);
  }
  int current=is_night(d)?0:d->uv_x10;
  Pt p=route_point(UV_PTS,2,uv_arc(current),NULL);
  if(d->weather_ok)p=draw_train(ctx,UV_PTS,2,uv_arc(current),NULL);
  if(d->weather_ok) {
    Pt peak=route_point(UV_PTS,2,uv_arc(d->uv_max_x10),NULL);
    draw_ring(ctx,peak,4);
  }
  char text[16];
  if(d->weather_ok)snprintf(text,sizeof(text),"UV %d",(current+5)/10);
  else snprintf(text,sizeof(text),"UV --");
  GRect r=ink_box(text,s_value,p.x,UV_Y+12,G24_H,GTextAlignmentCenter);
  r.origin.x=lab_clamp(r.origin.x,XL+10,XR-10-r.size.w);
  put(ctx,text,s_value,r,G24_TOP,C_UV);take(taken,r);
  label(ctx,"Low",XL+18,UV_Y-18,GTextAlignmentLeft);
  label(ctx,"High",XL+(XR-XL)/2,UV_Y-18,GTextAlignmentCenter);
  label(ctx,"11+",XR-12,UV_Y-18,GTextAlignmentRight);
  // Reserve the station/track band, then only the value's actual ink box.
  // Extending the full-width reservation below it hides the goal caption.
  take(taken,GRect(4,UV_Y-18,192,26));
}

static void draw_disruption(GContext *ctx,const LabData *d,Taken *taken) {
  bool critical=d->battery_pct<10 && !d->charging;
  if(!critical && d->bt_connected)return;
  char text[24];
  if(critical && !d->bt_connected)snprintf(text,sizeof(text),"%d%% / Phone",d->battery_pct);
  else if(critical)snprintf(text,sizeof(text),"Battery %d%%",d->battery_pct);
  else snprintf(text,sizeof(text),"Phone lost");
  int w=lab_text_size(text,s_small).w+8;
  GRect r=GRect((200-w)/2,73,w,15);
  graphics_context_set_fill_color(ctx,critical?C_ALERT:C_WARN);
  graphics_fill_rect(ctx,r,2,GCornersAll);
  lab_text(ctx,text,s_small,GRect(r.origin.x+4,72,w-8,18),GTextAlignmentLeft,C_WHITE);
  take(taken,r);
}

static void draw(GContext *ctx, GRect bounds, const LabData *d) {
  graphics_context_set_fill_color(ctx, C_PAPER);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  Taken taken = {.n = 0};
  DayMap m = day_map(d);
  int arch_len = arc_to(ARCH_PTS, ARCH_N - 1);

  // Lines: night - day - night on the day line when the sun times are known.
  if (m.known) {
    draw_section(ctx, DAY_PTS, DAY_N, 0, m.s_rise, C_NIGHT);
    draw_section(ctx, DAY_PTS, DAY_N, m.s_rise, m.s_set, C_DAY);
    draw_section(ctx, DAY_PTS, DAY_N, m.s_set, m.len, C_NIGHT);
  } else {
    draw_section(ctx, DAY_PTS, DAY_N, 0, m.len, C_DAY);
  }
  draw_section(ctx, ARCH_PTS, ARCH_N, 0, S_JCT, C_HEART);
  draw_section(ctx, ARCH_PTS, ARCH_N, S_JCT, arch_len, C_STEPS);
  GColor end_c = m.known ? C_NIGHT : C_DAY;
  draw_buffer(ctx, DAY_PTS[0], end_c);
  draw_buffer(ctx, DAY_PTS[DAY_N - 1], end_c);
  draw_buffer(ctx, ARCH_PTS[0], C_HEART);

  draw_header(ctx, d, &taken);
  draw_disruption(ctx,d,&taken);
  draw_day_stations(ctx, d, &m, &taken);

  // Steps stations: a tick every fifth of the goal; the goal terminus is a ring,
  // filled once you've arrived.
  int steps_len = arch_len - S_JCT;
  for (int i = 1; i < 5; i++) {
    int seg;
    Pt p = route_point(ARCH_PTS, ARCH_N, S_JCT + steps_len * i / 5, &seg);
    if (ARCH_PTS[seg].y == ARCH_PTS[seg + 1].y) {
      draw_tick(ctx, p, 0, 1, C_STEPS);
    } else if (ARCH_PTS[seg].x == ARCH_PTS[seg + 1].x) {
      draw_tick(ctx, p, -1, 0, C_STEPS);
    }
  }
  int32_t step_goal = d->step_goal > 0 ? d->step_goal : KIT_DEFAULT_STEP_GOAL;
  int32_t steps = d->steps_ok ? d->steps : 0;
  Pt goal = ARCH_PTS[ARCH_N - 1];
  draw_ring(ctx, goal, 6);
  bool arrived = steps >= step_goal;
  if (arrived) {
    graphics_context_set_fill_color(ctx, C_STEPS);
    graphics_fill_circle(ctx, GPoint(goal.x, goal.y), 3);
  }
  take(&taken, GRect(goal.x - 6, goal.y - 6, 13, 13));
  // Heart zone stations at 100 and 140 bpm.
  for (int bpm = 100; bpm <= 140; bpm += 40) {
    Pt p = route_point(ARCH_PTS, ARCH_N, S_JCT * (bpm - HR_LO) / (HR_HI - HR_LO), NULL);
    draw_tick(ctx, p, 1, 0, C_HEART);
  }
  Pt jct = ARCH_PTS[1];
  draw_ring(ctx, jct, 6);

  draw_uv(ctx, d, &taken);
  char nz[16];
  char wt[8];
  lab_fmt_time(wt, sizeof(wt), d->world_hour, d->world_min, d->is_24h);
  snprintf(nz, sizeof(nz), "%s %s", d->world_label, wt);
  GRect nzr = ink_box(nz, s_label, 100 + 7, Y_BOT - 5, G18_H, GTextAlignmentCenter);
  put(ctx, nz, s_label, nzr, G18_TOP, C_INK);
  blit(ctx, ICON_PLANE, 11, nzr.origin.x - 14, nzr.origin.y, C_INK);
  nzr.origin.x -= 14;
  nzr.size.w += 14;
  take(&taken, nzr);

  // Day train at now.
  draw_train(ctx, DAY_PTS, DAY_N, day_arc(&m, d->minute_of_day), NULL);

  // Steps train, with the count beside it.
  char t[16];
  int s_steps = S_JCT + (int)((int64_t)steps_len * lab_clamp(steps, 0, step_goal) / step_goal);
  int seg;
  Pt p = draw_train(ctx, ARCH_PTS, ARCH_N, lab_clamp(s_steps, S_JCT + 1500, arch_len - 1500),
                    &seg);
  if (d->steps_ok) {
    lab_fmt_thousands(t, sizeof(t), steps);
  } else {
    snprintf(t, sizeof(t), "--");
  }
  GRect r = ink_box(t, s_value, XR - 11, 145, G24_H, GTextAlignmentRight);
  if (ARCH_PTS[seg].x != ARCH_PTS[seg + 1].x) {
    r = ink_box(t, s_value, p.x, YS + 9, G24_H, GTextAlignmentCenter);
    r.origin.x = lab_clamp(r.origin.x, XL + BEND + 2, XR - BEND - 2 - r.size.w);
  }
  put(ctx, t, s_value, r, G24_TOP, C_INK);
  take(&taken, r);
  if (!arrived) {
    fmt_k(t, sizeof(t), step_goal);
    GRect g = ink_box(t, s_label, goal.x - 10, goal.y - 5, G18_H, GTextAlignmentRight);
    if (!hits(&taken, GRect(g.origin.x, g.origin.y - 4, g.size.w, g.size.h + 4))) {
      put(ctx, t, s_label, g, G18_TOP, C_INK);
    }
  }

  // Heart train, with the bpm beside it.
  int bx = XL + 10;
  int hy;
  if (d->hr_ok) {
    int s_hr = S_JCT * (lab_clamp(d->heart_rate, HR_LO, HR_HI) - HR_LO) / (HR_HI - HR_LO);
    Pt hp = draw_train(ctx, ARCH_PTS, ARCH_N, lab_clamp(s_hr, 900, S_JCT - 1500), NULL);
    snprintf(t, sizeof(t), "%d", d->heart_rate);
    hy = hp.y - G24_H / 2;
  } else {
    snprintf(t, sizeof(t), "--");
    hy = Y_BOT - 20;
  }
  r = ink_box(t, s_value, bx + 14, hy, G24_H, GTextAlignmentLeft);
  r.origin.x = bx;
  r.size.w += 14;
  r.origin.y=145;
  blit(ctx, ICON_HEART, 10, bx, r.origin.y + 2, C_HEART);
  lab_text(ctx, t, s_value, GRect(bx + 14, r.origin.y - G24_TOP, 60, 40), GTextAlignmentLeft,
           C_INK);
}

// Detail view: service status board ---------------------------------------------

typedef enum { ST_GOOD, ST_MINOR, ST_SEVERE, ST_INFO } Severity;

typedef struct {
  const char *name;
  GColor box;
  GColor box_text;
  const char *status;
  Severity sev;
  char detail[48];
} StatusRow;

// ETA at today's pace: steps per minute since the first active hour. This is
// Summit's formula (designs/21-summit), so every face reports the same ETA.
static int eta_minute(const LabData *d, int32_t goal) {
  if (!d->steps_ok || d->steps <= 0) {
    return -1;
  }
  int first = -1;
  for (int h = 0; h < 24; h++) {
    if (d->steps_by_hour[h] > 0) {
      first = h;
      break;
    }
  }
  int elapsed = d->minute_of_day - (first < 0 ? 0 : first * 60);
  if (elapsed < 1) {
    elapsed = 1;
  }
  int32_t left = goal - d->steps;
  return d->minute_of_day + (int)((left * elapsed) / d->steps);
}

static void status_rows(const LabData *d, StatusRow *rows) {
  char a[12];
  // Day.
  StatusRow *r = &rows[0];
  r->name = "Day";
  r->box = C_DAY;
  r->box_text = C_WHITE;
  bool sun_known=d->weather_ok && d->sunrise_min>0 && d->sunset_min>d->sunrise_min;
  bool daylight = sun_known && d->minute_of_day >= d->sunrise_min &&
                  d->minute_of_day < d->sunset_min;
  r->status = !sun_known ? "No information" : daylight ? "Good service" : "Night service";
  r->sev = ST_GOOD;
  if (sun_known && d->next_sun_min>=0) {
    int next = d->next_sun_min;
    int wait = next + 1440*d->next_sun_day_delta - d->minute_of_day;
    snprintf(r->detail, sizeof(r->detail), "%s in %dh%02d", d->next_sun_is_set ? "Sunset" : "Sunrise",
             wait / 60, wait % 60);
  } else {
    snprintf(r->detail, sizeof(r->detail), "Sun times unknown");
  }

  // Steps.
  r = &rows[1];
  r->name = "Steps";
  r->box = C_STEPS;
  r->box_text = C_WHITE;
  // The count is on the face, so the board gives the comparisons: against your
  // usual pace, and when the train is due at the goal terminus.
  char n[12], due[16];
  int32_t goal = d->step_goal > 0 ? d->step_goal : KIT_DEFAULT_STEP_GOAL;
  int32_t diff = d->steps - d->steps_typical;
  int eta = eta_minute(d, goal);
  due[0] = 0;
  if (eta >= 0 && eta < 1440) {
    lab_fmt_minute_of_day(a, sizeof(a), eta, d->is_24h);
    snprintf(due, sizeof(due), "due %s", a);
  }
  if (!d->steps_ok) {
    r->status = "No information";
    r->sev = ST_INFO;
    snprintf(r->detail, sizeof(r->detail), "No step data");
  } else if (d->steps >= goal) {
    r->status = "Goal reached";
    r->sev = ST_GOOD;
    lab_fmt_thousands(n, sizeof(n), d->steps);
    snprintf(r->detail, sizeof(r->detail), "%s · %d%% of goal", n, (int)(d->steps * 100 / goal));
  } else if (d->steps <= 0) {
    r->status = "Not yet running";
    r->sev = ST_INFO;
    lab_fmt_thousands(n, sizeof(n), goal);
    snprintf(r->detail, sizeof(r->detail), "0 of %s", n);
  } else {
    bool ahead = d->steps_typical <= 0 || diff >= 0;
    r->status = ahead ? "Good service"
                : diff * 5 >= -d->steps_typical ? "Minor delays" : "Severe delays";
    r->sev = ahead ? ST_GOOD : diff * 5 >= -d->steps_typical ? ST_MINOR : ST_SEVERE;
    kit_format_pace(r->detail,sizeof(r->detail),d->steps,d->steps_typical);
    if(due[0]) {
      char combined[80];
      snprintf(combined,sizeof(combined),"%s · %s",r->detail,due);
      size_t length=strlen(combined);
      if(length<sizeof(r->detail) && lab_text_size(combined,s_small).w<=130)
        memcpy(r->detail,combined,length+1);
    }
  }

  // Heart.
  r = &rows[2];
  r->name = "Heart";
  r->box = C_HEART;
  r->box_text = C_WHITE;
  if (!d->hr_ok) {
    r->status = "No information";
    r->sev = ST_INFO;
    if(d->hr_min>0 && d->hr_max>=d->hr_min)
      snprintf(r->detail,sizeof(r->detail),"-- bpm · %d–%d%s",d->hr_min,d->hr_max,
               d->hr_range_partial ? "*" : "");
    else snprintf(r->detail,sizeof(r->detail),"No reading%s",d->hr_range_partial ? "*" : "");
  } else {
    r->status = d->heart_rate >= 140 ? "Express service"
                : d->heart_rate >= 100 ? "Running fast" : "Good service";
    r->sev = ST_GOOD;
    if(d->hr_min>0 && d->hr_max>=d->hr_min)
      snprintf(r->detail,sizeof(r->detail),"%d bpm · %d–%d%s",d->heart_rate,d->hr_min,d->hr_max,
               d->hr_range_partial ? "*" : "");
    else snprintf(r->detail,sizeof(r->detail),"%d bpm · range --%s",d->heart_rate,
                  d->hr_range_partial ? "*" : "");
  }

  // UV service describes the WHO band; the open ring on the map is peak UV.
  r=&rows[3];r->name="UV";r->box=C_UV;r->box_text=C_WHITE;
  int uv=is_night(d)?0:((d->uv_x10+5)/10);
  r->status=!d->weather_ok?"No information":uv>=11?"Extreme":uv>=8?"Very high":
      uv>=6?"High":uv>=3?"Moderate":"Low";
  r->sev=!d->weather_ok?ST_INFO:uv>=6?ST_SEVERE:uv>=3?ST_MINOR:ST_GOOD;
  if(d->weather_ok)snprintf(r->detail,sizeof(r->detail),"UV %d · peak %d",uv,(d->uv_max_x10+5)/10);
  else snprintf(r->detail,sizeof(r->detail),"Forecast unavailable");

  // Weather.
  r = &rows[4];
  r->name = "Weather";
  r->box = GColorFromHEX(0x0055AA);
  r->box_text = C_WHITE;
  if (!d->weather_ok) {
    r->status = "No information";
    r->sev = ST_INFO;
    snprintf(r->detail, sizeof(r->detail), "Forecast unavailable");
  } else {
    LabWxKind k = lab_weather_kind(d->weather_code);
    int h = rain_peak_hour(d);
    if (k == LabWxStorm || k == LabWxSnow || d->rain_pct >= 70) {
      r->status = "Severe delays";
      r->sev = ST_SEVERE;
      snprintf(r->detail, sizeof(r->detail), "%s · %d°", lab_weather_text(d->weather_code),
               d->temp_fahrenheit?(d->temp*9/5+32):d->temp);
    } else if (h >= 0) {
      r->status = "Minor delays";
      r->sev = ST_MINOR;
      lab_fmt_minute_of_day(a, sizeof(a), d->rolling_hour[h] * 60, d->is_24h);
      snprintf(r->detail, sizeof(r->detail), "%d%% %s%s · %d°", d->rolling_rain[h], a, d->rolling_day[h]?"+1":"",
               d->temp_fahrenheit?(d->temp*9/5+32):d->temp);
    } else {
      r->status = "Good service";
      r->sev = ST_GOOD;
      snprintf(r->detail, sizeof(r->detail), "%s · %d°", lab_weather_text(d->weather_code),
               d->temp_fahrenheit?(d->temp*9/5+32):d->temp);
    }
  }

  // Battery.
  r = &rows[5];
  r->name = "Battery";
  r->box = GColorFromHEX(0xFFAA00);
  r->box_text = C_INK;
  int pct = d->battery_pct;
  if (d->charging) {
    r->status = "Planned works";
    r->sev = ST_INFO;
    snprintf(r->detail, sizeof(r->detail), "Charging · %d%%", pct);
  } else {
    r->status = d->battery_pct < 10 ? "Part suspended"
                : d->battery_pct < 40 ? "Reduced service" : "Good service";
    r->sev = d->battery_pct < 10 ? ST_SEVERE : d->battery_pct < 40 ? ST_MINOR : ST_GOOD;
    snprintf(r->detail, sizeof(r->detail), "%d%%", pct);
  }

  // Phone.
  r = &rows[6];
  r->name = "Phone";
  r->box = GColorFromHEX(0x555555);
  r->box_text = C_WHITE;
  r->status = d->bt_connected ? "Good service" : "Suspended";
  r->sev = d->bt_connected ? ST_GOOD : ST_SEVERE;
  snprintf(r->detail, sizeof(r->detail), d->bt_connected ? "Connected" : "Disconnected");
}

#define BOARD_TOP 23
#define BOARD_ROW 29
#define BOX_W 56
#define STATUS_X (4 + BOX_W + 6)

static void draw_detail(GContext *ctx, GRect bounds, const LabData *d) {
  graphics_context_set_fill_color(ctx, C_PAPER);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_antialiased(ctx, false);

  // Header band.
  graphics_context_set_fill_color(ctx, C_INK);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 21), 0, GCornerNone);
  lab_text(ctx, "Service status", s_label, GRect(6, -2, 130, 22), GTextAlignmentLeft, C_WHITE);
  char now[8];
  lab_fmt_time(now, sizeof(now), d->now.tm_hour, d->now.tm_min, d->is_24h);
  lab_text(ctx, now, s_label, GRect(130, -2, 64, 22), GTextAlignmentRight, C_WHITE);

  StatusRow rows[7];
  status_rows(d, rows);
  for (int i = 0; i < 7; i++) {
    int y = BOARD_TOP + i * BOARD_ROW;
    StatusRow *r = &rows[i];
    graphics_context_set_fill_color(ctx, r->box);
    graphics_fill_rect(ctx, GRect(4, y, BOX_W, 27), 0, GCornerNone);
    lab_text(ctx, r->name, s_small, GRect(6, y + 4, BOX_W - 2, 18), GTextAlignmentLeft,
             r->box_text);
    // Disruptions sit in a chip: brown-orange for minor, dark red for severe.
    if (r->sev == ST_MINOR || r->sev == ST_SEVERE) {
      int w = lab_text_size(r->status, s_label).w;
      graphics_context_set_fill_color(ctx, r->sev == ST_SEVERE ? C_ALERT : C_WARN);
      graphics_fill_rect(ctx, GRect(STATUS_X, y, w + 8, 16), 2, GCornersAll);
      lab_text(ctx, r->status, s_label, GRect(STATUS_X + 4, y - 5, 124, 22),
               GTextAlignmentLeft, C_WHITE);
    } else {
      lab_text(ctx, r->status, s_label, GRect(STATUS_X, y - 5, 124, 22), GTextAlignmentLeft,
               C_INK);
    }
    lab_text(ctx, r->detail, s_small, GRect(STATUS_X, y + 12, 196 - STATUS_X, 18), GTextAlignmentLeft,
             C_INK);
    if (i < 6) {
      graphics_context_set_fill_color(ctx, GColorFromHEX(0xAAAAAA));
      graphics_fill_rect(ctx, GRect(STATUS_X, y + 27, 196 - STATUS_X, 1), 0, GCornerNone);
    }
  }
}

const LabDesign design_underground2 = {"Underground", load, NULL, draw, draw_detail};
