"""Rebuild the Kivo v1 logo artwork and exports. Requires Pillow, fontTools, resvg-py."""
from pathlib import Path
import json
import zipfile
from io import BytesIO
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from resvg_py import svg_to_bytes

ROOT = Path(__file__).resolve().parent
VIOLET = '#171717'  # Retained variable name; the revised palette is strictly monochrome.
INK = '#171717'
LILAC = '#FFFFFF'
PAPER = '#F5F5F3'
FONT = Path('C:/Windows/Fonts/seguisb.ttf')

# Three optical parts: a film spine and two folded light beams.
STEM = 'M58 40H78Q90 40 90 52V204Q90 216 78 216H58Q46 216 46 204V52Q46 40 58 40Z'
UPPER = 'M109 108L169 48Q177 40 188 40H204Q219 40 209 52L149 117Q141 126 130 126H116Q99 126 109 108Z'
LOWER = 'M110 148Q100 132 117 132H131Q142 132 150 141L210 204Q221 216 205 216H187Q177 216 170 209Z'
PATHS = (STEM, UPPER, LOWER)


def mark(color=VIOLET, gradient=False):
    if gradient:
        fills = ('url(#violet)', 'url(#violet)', 'url(#beam)')
    else:
        fills = (color,) * 3
    return ''.join(f'<path fill="{fill}" d="{path}"/>' for fill, path in zip(fills, PATHS))


DEFS = ''  # No gradients in the revised identity.


def svg(body, width, height, title='Kivo logo', defs=''):
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img"><title>{title}</title>{defs}{body}</svg>'


font = TTFont(FONT)
glyphset = font.getGlyphSet()
cmap = font.getBestCmap()
upem = font['head'].unitsPerEm
x = 0
letters = []
for letter in 'kivo':
    glyph_name = cmap[ord(letter)]
    pen = SVGPathPen(glyphset)
    glyphset[glyph_name].draw(pen)
    letters.append(f'<path transform="translate({x},0)" d="{pen.getCommands()}"/>')
    x += font['hmtx'].metrics[glyph_name][0] - 20
WORD_PATH = ''.join(letters)
WORD_UNITS = x + 20


def wordmark(left, baseline, size, color):
    scale = size / upem
    return f'<g fill="{color}" transform="translate({left},{baseline}) scale({scale},-{scale})">{WORD_PATH}</g>'


def write_svg(name, source, png_width=None):
    (ROOT / f'{name}.svg').write_text(source, encoding='utf-8')
    png = svg_to_bytes(svg_string=source)
    image = Image.open(BytesIO(png)).convert('RGBA')
    if png_width:
        new_height = round(image.height * png_width / image.width)
        # Re-render vectors at export size rather than magnifying pixels.
        png = svg_to_bytes(svg_string=source, width=png_width, height=new_height)
    (ROOT / f'{name}.png').write_bytes(png)
    return source


symbol = write_svg('kivo-symbol', svg(mark(), 256, 256, 'Kivo symbol'), 1024)
write_svg('kivo-symbol-black', svg(mark(INK), 256, 256, 'Kivo monochrome dark symbol'), 1024)
write_svg('kivo-symbol-white', svg(mark("#FFFFFF"), 256, 256, 'Kivo monochrome white symbol'), 1024)
# Monochrome only: the superseded gradient variant is not part of this delivery.

horizontal = svg(f'<g transform="translate(16,0)">{mark()}</g>' + wordmark(286, 181, 172, INK), 646, 256, 'Kivo horizontal logo')
write_svg('kivo-logo-horizontal', horizontal, 1938)
write_svg('kivo-logo-horizontal-light', svg(f'<g transform="translate(16,0)">{mark(LILAC)}</g>' + wordmark(286, 181, 172, '#FFFFFF'), 646, 256, 'Kivo logo for dark backgrounds'), 1938)
write_svg('kivo-logo-horizontal-black', svg(f'<g transform="translate(16,0)">{mark(INK)}</g>' + wordmark(286, 181, 172, INK), 646, 256, 'Kivo single color logo'), 1938)
write_svg('kivo-logo-horizontal-white', svg(f'<g transform="translate(16,0)">{mark("#FFFFFF")}</g>' + wordmark(286, 181, 172, '#FFFFFF'), 646, 256, 'Kivo white logo'), 1938)

word_size = 145
word_width = WORD_UNITS * word_size / upem
write_svg('kivo-logo-stacked', svg(f'<g transform="translate(52,0)">{mark()}</g>' + wordmark((360-word_width)/2, 356, word_size, INK), 360, 402, 'Kivo stacked logo'), 1080)

app_body = '<rect x="0" y="0" width="512" height="512" rx="114" fill="#171717"/>' + f'<g transform="translate(55,49) scale(1.58)">{mark("#FFFFFF")}</g>'
app = write_svg('kivo-app-icon', svg(app_body, 512, 512, 'Kivo app icon rounded', DEFS), 1024)
app_flat = write_svg('kivo-app-icon-flat', svg('<rect width="512" height="512" rx="114" fill="#171717"/>' + f'<g transform="translate(55,49) scale(1.58)">{mark("#FFFFFF")}</g>', 512, 512, 'Kivo app icon flat'), 1024)
# Square background for launchers that apply their own adaptive mask.
write_svg('kivo-app-icon-square', svg('<rect width="512" height="512" fill="#171717"/>' + f'<g transform="translate(55,49) scale(1.58)">{mark("#FFFFFF")}</g>', 512, 512, 'Kivo app icon unmasked', DEFS), 1024)

master = svg_to_bytes(svg_string=svg(app_body, 512, 512, 'Kivo app icon', DEFS), width=1024, height=1024)
master_image = Image.open(BytesIO(master)).convert('RGBA')
for side in (16, 24, 32, 48, 64, 96, 128, 192, 256, 512, 1024):
    master_image.resize((side, side), Image.Resampling.LANCZOS).save(ROOT / f'kivo-app-{side}.png')
master_image.save(ROOT / 'kivo-app.ico', format='ICO', sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
master_image.save(ROOT / 'favicon.ico', format='ICO', sizes=[(16,16),(32,32),(48,48)])

# Android adaptive icon layers use native VectorDrawable geometry.
colors = ['#FFFFFF'] * 3
android_paths = ''.join(f'<path android:fillColor="{c}" android:pathData="{p}"/>' for c,p in zip(colors, PATHS))
(ROOT / 'ic_launcher_foreground.xml').write_text('<?xml version="1.0" encoding="utf-8"?>\n<vector xmlns:android="http://schemas.android.com/apk/res/android" android:width="108dp" android:height="108dp" android:viewportWidth="432" android:viewportHeight="432"><group android:translateX="88" android:translateY="88">' + android_paths + '</group></vector>\n', encoding='utf-8')
(ROOT / 'ic_launcher_background.xml').write_text('<?xml version="1.0" encoding="utf-8"?>\n<vector xmlns:android="http://schemas.android.com/apk/res/android" android:width="108dp" android:height="108dp" android:viewportWidth="108" android:viewportHeight="108"><path android:fillColor="#171717" android:pathData="M0,0H108V108H0Z"/></vector>\n', encoding='utf-8')
(ROOT / 'ic_launcher_monochrome.xml').write_text((ROOT / 'ic_launcher_foreground.xml').read_text(encoding='utf-8'), encoding='utf-8')
# Android ready-to-copy resource tree (API 26+, themed icon layer API 33+).
android_root = ROOT / 'android-res'
assert ROOT.is_dir()
for resource_dir in ('drawable', 'mipmap-anydpi-v26', 'mipmap-anydpi-v33'):
    target = android_root / resource_dir
    target.mkdir(parents=True, exist_ok=True)
for resource_name in ('ic_launcher_foreground.xml','ic_launcher_background.xml','ic_launcher_monochrome.xml'):
    (android_root / 'drawable' / resource_name).write_bytes((ROOT / resource_name).read_bytes())
adaptive_base = '<?xml version="1.0" encoding="utf-8"?>\n<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android"><background android:drawable="@drawable/ic_launcher_background"/><foreground android:drawable="@drawable/ic_launcher_foreground"/>{extra}</adaptive-icon>\n'
(android_root / 'mipmap-anydpi-v26' / 'ic_launcher.xml').write_text(adaptive_base.format(extra=''), encoding='utf-8')
(android_root / 'mipmap-anydpi-v33' / 'ic_launcher.xml').write_text(adaptive_base.format(extra='<monochrome android:drawable="@drawable/ic_launcher_monochrome"/>'), encoding='utf-8')
for density, side in (('mdpi',48),('hdpi',72),('xhdpi',96),('xxhdpi',144),('xxxhdpi',192)):
    density_dir = android_root / ('mipmap-' + density)
    density_dir.mkdir(exist_ok=True)
    master_image.resize((side,side),Image.Resampling.LANCZOS).save(density_dir / 'ic_launcher.png')
(ROOT / 'brand-colors.json').write_text(json.dumps({'brand':'kivo', 'concept':'K / 纯黑白影院', 'version':'1.1-monochrome', 'primary':INK,'inverse':'#FFFFFF','paper':PAPER,'neutral_gray':'#777777','effects':'无彩色、无渐变、无光效','minimum_symbol_size_px':24,'minimum_horizontal_logo_width_px':110,'clear_space':'至少标志左侧竖笔画宽度（44 / 256 画布单位）','type_note':'字标已转曲，不依赖系统字体；以 Segoe UI Semibold 为基础，单字微调间距。','scope_note':'独立设计提案；未改动应用源码，未完成商标近似检索。'}, ensure_ascii=False, indent=2), encoding='utf-8')

# A compact raster reference is also used as a fidelity guide for the generated concept board.
board = Image.new('RGB', (1600, 1120), PAPER)
draw = ImageDraw.Draw(board)
font_title = ImageFont.truetype(str(FONT), 24)
font_small = ImageFont.truetype(str(FONT), 16)
draw.text((66, 43), 'KIVO / MONOCHROME', fill=INK, font=font_title)
draw.text((66, 81), 'BRAND IDENTITY / 01', fill='#777777', font=font_small)
logo_img = Image.open(ROOT / 'kivo-logo-horizontal.png').convert('RGBA')
logo_img.thumbnail((800,320))
board.paste(logo_img,(400,195),logo_img)
draw.rounded_rectangle((66,500,760,900),radius=28,fill='#FFFFFF')
draw.rounded_rectangle((790,500,1534,900),radius=28,fill='#171717')
light_logo = Image.open(ROOT / 'kivo-logo-horizontal-light.png').convert('RGBA')
light_logo.thumbnail((590,250))
board.paste(light_logo,(866,571),light_logo)
icon_img = Image.open(ROOT / 'kivo-app-icon.png').convert('RGBA').resize((214,214),Image.Resampling.LANCZOS)
board.paste(icon_img,(126,568),icon_img)
mark_img = Image.open(ROOT / 'kivo-symbol.png').convert('RGBA').resize((214,214),Image.Resampling.LANCZOS)
board.paste(mark_img,(465,568),mark_img)
draw.text((126,824),'APP ICON',fill='#777777',font=font_small)
draw.text((465,824),'CORE SYMBOL',fill='#777777',font=font_small)
draw.text((866,824),'DARK BACKGROUND',fill='#BDBDBD',font=font_small)
for i,(color,label) in enumerate([(INK,'171717'),('#777777','777777'),('#D8D8D5','D8D8D5'),('#FFFFFF','FFFFFF')]):
    x0=66+i*374
    draw.rounded_rectangle((x0,953,x0+350,1004),radius=12,fill=color)
    draw.text((x0,1020),'#'+label,fill='#777777',font=font_small)
board.save(ROOT / 'kivo-brand-preview.png')

# Zip only deliverables (not generated moodboard; that can be supplied separately).
def package():
    with zipfile.ZipFile(ROOT / 'kivo-logo-kit.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        for path in sorted(ROOT.rglob('*')):
            allowed = path.name.startswith(('kivo-', 'ic_launcher', 'brand-colors', 'favicon')) or path.suffix == '.html'
            superseded = '-gradient' in path.name or path.name.startswith(('android-', 'Create_', 'concept-'))
            if path.is_file() and path.suffix.lower() in ('.svg','.png','.ico','.xml','.json','.html') and allowed and not superseded:
                z.write(path, path.relative_to(ROOT).as_posix())
package()
print(json.dumps({'output_dir':str(ROOT),'files':len(list(ROOT.iterdir())),'reference':str(ROOT / 'kivo-brand-preview.png')}, ensure_ascii=False))
