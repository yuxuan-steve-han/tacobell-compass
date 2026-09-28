"""Rasterise the Taco Bell SVG into a 1-bit bitmap header (tacobell_logo.h).

    python tools/make_logo.py Taco_Bell_2023.svg tacobell_logo.h [--width 48] [--preview logo.png]

Only needs the standard library: path data (M/L/H/V/C/Z) is flattened and filled with
the nonzero rule, 4x4 supersampled, then thresholded at 50% coverage.
"""
import argparse, re, struct, zlib

SS = 4
CURVE_STEPS = 16


def parse_path(d):
    toks = re.findall(r'[a-zA-Z]|-?(?:\d+\.?\d*|\.\d+)(?:e-?\d+)?', d)
    i = 0
    polys, cur = [], []
    x = y = sx = sy = 0.0
    cmd = None

    def num():
        nonlocal i
        i += 1
        return float(toks[i - 1])

    while i < len(toks):
        if toks[i].isalpha():
            cmd = toks[i]
            i += 1
            if cmd in 'zZ':
                if cur:
                    polys.append(cur)
                cur, x, y = [], sx, sy
                continue
        rel, c = cmd.islower(), cmd.lower()
        if c == 'm':
            if cur:
                polys.append(cur)
            dx, dy = num(), num()
            x, y = (x + dx, y + dy) if rel else (dx, dy)
            sx, sy, cur = x, y, [(x, y)]
            cmd = 'l' if rel else 'L'  # extra pairs after M are line-tos
        elif c == 'l':
            dx, dy = num(), num()
            x, y = (x + dx, y + dy) if rel else (dx, dy)
            cur.append((x, y))
        elif c == 'h':
            v = num()
            x = x + v if rel else v
            cur.append((x, y))
        elif c == 'v':
            v = num()
            y = y + v if rel else v
            cur.append((x, y))
        elif c == 'c':
            p = [num() for _ in range(6)]
            if rel:
                p = [p[k] + (x if k % 2 == 0 else y) for k in range(6)]
            for k in range(1, CURVE_STEPS + 1):
                t = k / CURVE_STEPS
                u = 1 - t
                cur.append((u**3 * x + 3*u*u*t * p[0] + 3*u*t*t * p[2] + t**3 * p[4],
                            u**3 * y + 3*u*u*t * p[1] + 3*u*t*t * p[3] + t**3 * p[5]))
            x, y = p[4], p[5]
        else:
            raise ValueError(f'unsupported path command {cmd!r}')
    if cur:
        polys.append(cur)
    return polys


def winding(polys, px, py):
    wn = 0
    for poly in polys:
        for (x1, y1), (x2, y2) in zip(poly, poly[1:] + poly[:1]):
            if (y1 <= py < y2 or y2 <= py < y1) and x1 + (py - y1) * (x2 - x1) / (y2 - y1) > px:
                wn += 1 if y2 > y1 else -1
    return wn


def rasterise(svg, width):
    vb = [float(v) for v in re.search(r'viewBox="([^"]+)"', svg).group(1).split()]
    vbx, vby, vbw, vbh = vb
    height = round(width * vbh / vbw)
    shapes = [parse_path(d) for d in re.findall(r'\sd="([^"]+)"', svg)]
    sx, sy = vbw / width, vbh / height
    grid = []
    for r in range(height):
        row = []
        for c in range(width):
            hits = sum(
                any(winding(s, vbx + (c + (b + .5) / SS) * sx, vby + (r + (a + .5) / SS) * sy) for s in shapes)
                for a in range(SS) for b in range(SS))
            row.append(hits * 2 >= SS * SS)
        grid.append(row)
    return grid


def write_header(grid, path, source):
    w, h = len(grid[0]), len(grid)
    stride = (w + 7) // 8
    lines = [f'// Generated from {source} by tools/make_logo.py ({w}x{h}, 1 bit per pixel, MSB first).',
             '#pragma once', '#include <stdint.h>', '',
             f'#define TB_LOGO_W {w}', f'#define TB_LOGO_H {h}', f'#define TB_LOGO_STRIDE {stride}', '',
             'static const uint8_t TB_LOGO[TB_LOGO_H * TB_LOGO_STRIDE] = {']
    for row in grid:
        bits = row + [False] * (stride * 8 - w)
        lines.append('  ' + ', '.join(
            f'0x{sum(1 << (7 - j) for j in range(8) if bits[k + j]):02X}' for k in range(0, len(bits), 8)) + ',')
    lines.append('};')
    open(path, 'w', newline='\n').write('\n'.join(lines) + '\n')


def write_preview(grid, path, scale=6):
    fg, bg = b'\xff\xff\xff', b'\x36\x39\x9a'
    raw = b''.join((b'\x00' + b''.join((fg if v else bg) * scale for v in row)) * scale for row in grid)

    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data))

    ihdr = struct.pack('>IIBBBBB', len(grid[0]) * scale, len(grid) * scale, 8, 2, 0, 0, 0)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) +
                           chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('svg')
    ap.add_argument('out')
    ap.add_argument('--width', type=int, default=48)
    ap.add_argument('--preview', help='also write an enlarged PNG preview')
    args = ap.parse_args()

    grid = rasterise(open(args.svg, encoding='utf-8').read(), args.width)
    write_header(grid, args.out, args.svg.replace('\\', '/').split('/')[-1])
    if args.preview:
        write_preview(grid, args.preview)
    print(f'{args.out}: {len(grid[0])}x{len(grid)}')
