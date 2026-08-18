"""Generate anti-aliased LVGL assets from the exact v1.1 vector geometry."""

from pathlib import Path
from PIL import Image, ImageDraw
import math

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src/display/ui/concept/assets"
SCALE = 8


def quad(p0, p1, p2, steps=64):
    return [((1-t)**2*p0[0] + 2*(1-t)*t*p1[0] + t*t*p2[0],
             (1-t)**2*p0[1] + 2*(1-t)*t*p1[1] + t*t*p2[1]) for t in (i/steps for i in range(steps+1))]


def cubic(p0, p1, p2, p3, steps=48):
    return [((1-t)**3*p0[0] + 3*(1-t)**2*t*p1[0] + 3*(1-t)*t*t*p2[0] + t**3*p3[0],
             (1-t)**3*p0[1] + 3*(1-t)**2*t*p1[1] + 3*(1-t)*t*t*p2[1] + t**3*p3[1]) for t in (i/steps for i in range(steps+1))]


def canvas(size):
    return Image.new("RGBA", (size[0]*SCALE, size[1]*SCALE), (0, 0, 0, 0))


def points(values):
    return [(round(x*SCALE), round(y*SCALE)) for x, y in values]


def sleeping_eye():
    im = canvas((38, 18)); d = ImageDraw.Draw(im)
    d.line(points(quad((3,14),(19,2),(35,14))), fill=(212,170,58,255), width=round(2.8*SCALE), joint="curve")
    d.line(points(quad((10,15),(19,10),(28,15))), fill=(212,170,58,115), width=round(1.4*SCALE), joint="curve")
    return im


def heart():
    im = canvas((34, 32)); d = ImageDraw.Draw(im)
    path = [(17,28)]
    path += cubic((17,28),(17,28),(2,18),(2,9))[1:]
    path += cubic((2,9),(2,4.58),(7.1,1.9),(11.3,4.2))[1:]
    path += cubic((11.3,4.2),(13.5,5.4),(17,6),(17,6))[1:]
    path += cubic((17,6),(17,6),(20.5,5.4),(22.7,4.2))[1:]
    path += cubic((22.7,4.2),(26.9,1.9),(32,4.58),(32,9))[1:]
    path += cubic((32,9),(32,18),(17,28),(17,28))[1:]
    d.polygon(points(path), fill=(232,96,122,255))
    return im


def tongue():
    im = canvas((20,15)); d = ImageDraw.Draw(im)
    path = quad((3,3),(10,0),(17,3))
    path += quad((17,3),(17,13),(10,13))[1:]
    path += quad((10,13),(3,13),(3,3))[1:]
    d.polygon(points(path), fill=(90,180,212,204))
    return im


def smile():
    im = canvas((44,20)); d = ImageDraw.Draw(im)
    d.line(points(quad((5,5),(22,20),(39,5))), fill=(255,255,255,102), width=round(2.2*SCALE), joint="curve")
    return im


def mini_heart():
    im = canvas((10,10)); d = ImageDraw.Draw(im)
    path = [(5,8.5)]
    path += cubic((5,8.5),(5,8.5),(.5,5.5),(.5,2.5))[1:]
    path += cubic((.5,2.5),(.5,.3),(3,.0),(5,1.5))[1:]
    path += cubic((5,1.5),(7,.0),(9.5,.3),(9.5,2.5))[1:]
    path += cubic((9.5,2.5),(9.5,5.5),(5,8.5),(5,8.5))[1:]
    d.polygon(points(path), fill=(232,96,122,179))
    return im


def standby_hint():
    im = canvas((24,28)); d = ImageDraw.Draw(im)
    white = (255,255,255,255)
    d.line(points([(12,2),(12,12)]), fill=white, width=round(1.3*SCALE))
    d.line(points([(7,8),(12,2),(17,8)]), fill=white, width=round(1.3*SCALE), joint="curve")
    d.rounded_rectangle((4*SCALE,14*SCALE,20*SCALE,26*SCALE), radius=4*SCALE,
                        outline=white, width=round(1.2*SCALE))
    d.ellipse((10*SCALE,18*SCALE,14*SCALE,22*SCALE), fill=(255,255,255,128))
    return im


def stroke(draw, path, color=(255,255,255,230), width=1.5):
    draw.line(points(path), fill=color, width=max(1, round(width*SCALE)), joint="curve")


def menu_cup():
    im = canvas((36,36)); d = ImageDraw.Draw(im)
    cup = [(7,15),(25,15),(25,24)] + quad((25,24),(25,29),(20,29))[1:] + [(12,29)] + quad((12,29),(7,29),(7,24))[1:] + [(7,15)]
    stroke(d, cup, width=1.6)
    handle = [(25,17)] + cubic((25,17),(33,17),(33,23),(25,23))[1:]
    stroke(d, handle, width=1.5)
    stroke(d, [(11,15),(11,29)], (255,255,255,46), .7)
    for x in (13,18,23): stroke(d, [(x,11),(x,10),(x+1.2,8.4)], (255,255,255,103), 1.2)
    return im


def menu_drops():
    im = canvas((36,36)); d = ImageDraw.Draw(im)
    for cx, top, radius, bottom in ((11,10.5,3.5,21),(18,13,4,25),(25,10.5,3.5,21)):
        path = [(cx,bottom), (cx-radius,14 if radius==3.5 else 17)]
        path += cubic(path[-1],(cx-radius,top),(cx+radius,top),(cx+radius,14 if radius==3.5 else 17))[1:]
        path += [(cx,bottom)]
        stroke(d, path, width=1.5)
    return im


def menu_water():
    im = canvas((36,36)); d = ImageDraw.Draw(im)
    for cx, cy in ((11,13),(25,13),(18,21)):
        path = [(cx,cy+7),(cx-4,cy+1)]
        path += cubic(path[-1],(cx-4,cy-3),(cx+4,cy-3),(cx+4,cy+1))[1:]
        path += [(cx,cy+7)]
        stroke(d, path, width=1.5)
    return im


def menu_bean():
    im = canvas((36,36)); d = ImageDraw.Draw(im)
    import math
    angle = math.radians(-25)
    ellipse = []
    for i in range(97):
        a = 2*math.pi*i/96
        x, y = 11*math.cos(a), 7*math.sin(a)
        ellipse.append((18+x*math.cos(angle)-y*math.sin(angle),18+x*math.sin(angle)+y*math.cos(angle)))
    stroke(d, ellipse, width=1.6)
    stroke(d, cubic((13,13.5),(14.5,16),(14.5,20),(13,22.5)), (255,255,255,128), 1.3)
    stroke(d, cubic((23,13.5),(21.5,16),(21.5,20),(23,22.5)), (255,255,255,128), 1.3)
    return im


def timer_icon():
    im = canvas((22,24)); d = ImageDraw.Draw(im)
    d.ellipse((3*SCALE,6*SCALE,19*SCALE,22*SCALE), outline=(255,255,255,255), width=round(1.3*SCALE))
    stroke(d, [(11,14),(11,9)], width=1.4)
    stroke(d, [(11,14),(14.5,16.5)], (255,255,255,153), 1.2)
    stroke(d, [(7.5,2),(14.5,2)], (255,255,255,102), 1.2)
    stroke(d, [(11,2),(11,5)], (255,255,255,102), 1.2)
    return im


def pressure_icon():
    im = canvas((11,11)); d = ImageDraw.Draw(im)
    d.ellipse((1.5*SCALE,1.5*SCALE,9.5*SCALE,9.5*SCALE), outline=(255,255,255,255), width=max(1,round(.9*SCALE)))
    d.ellipse((4*SCALE,4*SCALE,7*SCALE,7*SCALE), fill=(255,255,255,255))
    return im


def target_icon():
    im = canvas((12,12)); d = ImageDraw.Draw(im)
    stroke(d, [(1,6),(11,6)], width=.9); stroke(d, [(6,1),(6,11)], width=.9)
    stroke(d, [(1,2.5),(3,2.5),(3,1)], (255,255,255,128), .9)
    return im


def play_icon():
    im = canvas((14,16)); d = ImageDraw.Draw(im)
    d.polygon(points([(2,1.5),(12,8),(2,14.5)]), fill=(255,255,255,255))
    return im


def stop_icon():
    im = canvas((13,13)); d = ImageDraw.Draw(im)
    d.rounded_rectangle((2*SCALE,2*SCALE,11*SCALE,11*SCALE), radius=2*SCALE, fill=(220,70,70,230))
    return im


def up_hint():
    im = canvas((12,8)); d = ImageDraw.Draw(im)
    stroke(d, [(1,7),(6,2),(11,7)], (255,255,255,217), 1.5)
    return im


def down_hint():
    im = canvas((14,9)); d = ImageDraw.Draw(im)
    stroke(d, [(1,1),(7,7),(13,1)], width=1.2)
    return im


def heat_icon():
    im = canvas((22,22)); d = ImageDraw.Draw(im)
    for y in (6,12,18):
        path = cubic((3,y),(4.5,y-2),(6,y-2),(7.5,y))
        path += cubic((7.5,y),(9,y+2),(10.5,y+2),(12,y))[1:]
        path += cubic((12,y),(13.5,y-2),(15,y-2),(16.5,y))[1:]
        stroke(d, path, width=1.4)
    return im


def drop_icon():
    im = canvas((20,26)); d = ImageDraw.Draw(im)
    path = [(10,2)] + cubic((10,2),(8,5),(2,11),(2,16))[1:]
    path += cubic((2,16),(2,21),(5.6,24),(10,24))[1:]
    path += cubic((10,24),(14.4,24),(18,21),(18,16))[1:]
    path += cubic((18,16),(18,11),(12,5),(10,2))[1:]
    stroke(d, path, width=1.3)
    stroke(d, cubic((6,18.5),(6,20.5),(7.8,22),(10,22)), (255,255,255,115), 1.1)
    return im


def steam_icon():
    im = canvas((22,24)); d = ImageDraw.Draw(im)
    stroke(d, cubic((4,20),(5,16),(8,15),(8,11)), width=1.4)
    stroke(d, cubic((11,22),(12,17),(15,16),(15,11)), width=1.4)
    stroke(d, cubic((18,20),(18,16),(15,15),(15,11)), width=1.4)
    return im


def water_icon():
    im = canvas((22,24)); d = ImageDraw.Draw(im)
    for cx, top, bottom in ((6,5,14),(16,5,14),(11,11,20)):
        path = [(cx,bottom),(cx-3,top+3)]
        path += cubic(path[-1],(cx-3,top),(cx+3,top),(cx+3,top+3))[1:]
        path += [(cx,bottom)]
        stroke(d, path, width=1.3)
    return im


def grind_icon():
    im = canvas((24,24)); d = ImageDraw.Draw(im)
    d.ellipse((7*SCALE,4*SCALE,17*SCALE,16*SCALE), outline=(255,255,255,255), width=round(1.3*SCALE))
    stroke(d, cubic((7,14),(7,17),(9.2,19),(12,19)), width=1.3)
    stroke(d, cubic((12,19),(14.8,19),(17,17),(17,14)), width=1.3)
    stroke(d, cubic((9,8),(9.5,6.5),(10.5,6),(12,6)), (255,255,255,115), 1.1)
    stroke(d, [(3,20),(21,20)], (255,255,255,77), 1.1)
    return im


def spinner_icon():
    im = canvas((12,12)); d = ImageDraw.Draw(im)
    box = (1.5*SCALE,1.5*SCALE,10.5*SCALE,10.5*SCALE)
    d.arc(box, start=-90, end=50, fill=(255,255,255,255), width=round(1.3*SCALE))
    return im


def power_icon():
    im = canvas((22,22)); d = ImageDraw.Draw(im)
    stroke(d, [(11,3),(11,11)], width=1.5)
    path = cubic((7,5.4),(1.5,9),(4.2,19),(11,19))
    path += cubic((11,19),(17.8,19),(20.5,9),(15,5.4))[1:]
    stroke(d, path, width=1.4)
    return im


def wifi_icon():
    im = canvas((14,12)); d = ImageDraw.Draw(im)
    stroke(d, cubic((1,4.5),(4.2,1.2),(9.8,1.2),(13,4.5)), width=1.1)
    stroke(d, cubic((3.5,7),(5.4,5),(8.6,5),(10.5,7)), width=1.1)
    d.ellipse((6*SCALE,9*SCALE,8*SCALE,11*SCALE), fill=(255,255,255,255))
    return im


def bluetooth_icon():
    im = canvas((9,14)); d = ImageDraw.Draw(im)
    stroke(d, [(4.5,1),(4.5,13)], width=1.1)
    stroke(d, [(1.5,3.5),(7,7),(1.5,11)], width=1.1)
    stroke(d, [(7,3.5),(1.5,7),(7,10.5)], width=1.1)
    return im


def profile_arrow(left):
    im = canvas((5,9)); d = ImageDraw.Draw(im)
    path = [(4,1),(1,4.5),(4,8)] if left else [(1,1),(4,4.5),(1,8)]
    stroke(d, path, width=1.0)
    return im


def rgba565(image):
    image = image.resize((image.width//SCALE, image.height//SCALE), Image.Resampling.LANCZOS)
    data = bytearray()
    for r, g, b, a in image.getdata():
        value = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        data.extend((value & 0xff, value >> 8, a))
    return image.width, image.height, data


def rgb565(image):
    # Floyd-Steinberg diffusion removes broad RGB565 bands without the coarse
    # random grain of the previous renderer. Serpentine rows keep the tiny
    # quantisation texture isotropic on the round 480px LCD.
    width, height = image.size
    pixels = [list(map(float, px)) for px in image.convert("RGB").getdata()]
    packed = [0] * (width * height)
    levels = (31, 63, 31)
    for y in range(height):
        reverse = (y & 1) != 0
        xs = range(width - 1, -1, -1) if reverse else range(width)
        direction = -1 if reverse else 1
        for x in xs:
            index = y * width + x
            old = pixels[index]
            q = [max(0, min(levels[c], round(old[c] * levels[c] / 255.0))) for c in range(3)]
            restored = [q[c] * 255.0 / levels[c] for c in range(3)]
            packed[index] = (q[0] << 11) | (q[1] << 5) | q[2]
            error = [old[c] - restored[c] for c in range(3)]
            for dx, dy, weight in ((direction, 0, 7/16), (-direction, 1, 3/16), (0, 1, 5/16), (direction, 1, 1/16)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < width and ny < height:
                    target = pixels[ny * width + nx]
                    for c in range(3): target[c] += error[c] * weight
    data = bytearray()
    for value in packed:
        data.extend((value & 0xff, value >> 8))
    return image.width, image.height, data


def mix(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


def smoothstep(t):
    t = max(0.0, min(1.0, t)); return t*t*(3-2*t)


def state_background(ground, glow):
    # Bottom half of the two Figma radial layers. The base gradient has a
    # 60.192 * 10 radius around (240, 552); the colored glow uses the exported
    # 697.65 x 346.87 transform around the lower edge of the screen.
    im = Image.new("RGB", (480,240)); px = im.load()
    for yy in range(240):
        y = yy + 240
        for x in range(480):
            d = math.sqrt(((x-240)/601.92)**2 + ((y-552)/601.92)**2)
            if d <= .22: base = ground
            elif d <= .55: base = mix(ground, (10,10,10), smoothstep((d-.22)/.33))
            else: base = mix((10,10,10), (5,5,5), smoothstep((d-.55)/.45))
            gd = math.sqrt(((x-240)/697.65)**2 + ((y-519)/346.87)**2)
            alpha = glow[3] * max(0.0, 1.0 - smoothstep(gd))
            result = mix(base, glow[:3], alpha)
            # The stored asset starts at y=240. Feather its top into the screen
            # base so the cropped radial layer has no horizontal seam.
            if yy < 72: result = mix((5,5,5), result, smoothstep(yy / 72.0))
            px[x,yy] = result
    return im


def write_asset(name, image):
    width, height, data = rgba565(image)
    rows = [", ".join(f"0x{x:02x}" for x in data[i:i+18]) for i in range(0, len(data), 18)]
    body = ",\n    ".join(rows)
    source = f'''#include <lvgl.h>

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t concept_{name}_map[] = {{
    {body}
}};

const lv_img_dsc_t concept_{name} = {{
    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = {width},
    .header.h = {height},
    .data_size = {width * height} * LV_IMG_PX_SIZE_ALPHA_BYTE,
    .data = concept_{name}_map,
}};
'''
    (OUT / f"concept_{name}.c").write_text(source, encoding="utf-8")


def write_rgb_asset(name, image):
    width, height, data = rgb565(image)
    rows = [", ".join(f"0x{x:02x}" for x in data[i:i+20]) for i in range(0, len(data), 20)]
    body = ",\n    ".join(rows)
    source = f'''#include <lvgl.h>
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t concept_{name}_map[] = {{
    {body}
}};
const lv_img_dsc_t concept_{name} = {{
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .header.always_zero = 0, .header.reserved = 0,
    .header.w = {width}, .header.h = {height},
    .data_size = {width * height * 2},
    .data = concept_{name}_map,
}};
'''
    (OUT / f"concept_{name}.c").write_text(source, encoding="utf-8")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for name, image in {
        "sleeping_eye": sleeping_eye(),
        "heart_eye": heart(),
        "tongue": tongue(),
        "smile": smile(),
        "mini_heart": mini_heart(),
        "standby_hint": standby_hint(),
        "menu_cup": menu_cup(),
        "menu_drops": menu_drops(),
        "menu_water": menu_water(),
        "menu_bean": menu_bean(),
        "timer": timer_icon(),
        "pressure": pressure_icon(),
        "target": target_icon(),
        "play": play_icon(),
        "stop": stop_icon(),
        "up_hint": up_hint(),
        "down_hint": down_hint(),
        "heat": heat_icon(),
        "drop": drop_icon(),
        "steam": steam_icon(),
        "water": water_icon(),
        "grind": grind_icon(),
        "spinner": spinner_icon(),
        "power": power_icon(),
        "wifi": wifi_icon(),
        "bluetooth": bluetooth_icon(),
        "profile_left": profile_arrow(True),
        "profile_right": profile_arrow(False),
    }.items():
        write_asset(name, image)
    states = {
        # These are permanent background layers. UI and charts are composed
        # above them; heating only breathes via a subtle whole-image opacity.
        "bg_heating": ((0,17,46),(15,70,215,.52)), "bg_ready": ((0,24,32),(15,155,190,.44)),
        "bg_preinfuse": ((30,9,0),(175,65,0,.54)), "bg_brewing": ((40,0,0),(188,8,8,.60)),
        "bg_done": ((0,26,14),(18,175,75,.42)), "bg_steam": ((0,14,34),(15,88,200,.50)),
        "bg_water": ((0,16,32),(15,100,208,.46)), "bg_grind": ((26,16,0),(160,110,10,.50)),
    }
    for name, (ground, glow) in states.items(): write_rgb_asset(name, state_background(ground, glow))


if __name__ == "__main__":
    main()
