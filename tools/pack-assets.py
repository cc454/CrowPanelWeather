"""Threshold rendered originals and export portable, MSB-first 1-bpp assets."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps
import json, math, argparse, os

parser = argparse.ArgumentParser()
parser.add_argument('--current-height', type=int, default=48)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
assets = root / 'assets'
metadata = {}
all_rows = []

def pack(image):
    stride = (image.width + 7) // 8
    data = bytearray(stride * image.height)
    for y in range(image.height):
        for x in range(image.width):
            if image.getpixel((x,y)) == 0:
                data[y*stride+x//8] |= 0x80 >> (x%8)
    return data

def emit_header(label, items):
    lines = ['// Generated; black=1, white/transparent=0; row-major, MSB first.',
             '#pragma once', '#include <stdint.h>',
             '#if defined(ARDUINO)', '#include <Arduino.h>',
             '#define CPW_ASSET_FLASH PROGMEM', '#else', '#define CPW_ASSET_FLASH', '#endif',
             'namespace cpw_assets {']
    for name, image in items:
        ident = name.replace('-', '_')+'_'+label
        data = pack(image)
        lines += [f'static const uint16_t {ident}_width = {image.width};',
                  f'static const uint16_t {ident}_height = {image.height};',
                  f'static const uint8_t {ident}[] CPW_ASSET_FLASH = {{']
        lines += ['  '+','.join(f'0x{b:02x}' for b in data[i:i+16])+',' for i in range(0,len(data),16)]
        lines += ['};']
        # Reconstruct every packed pixel, checking byte order and white padding.
        for y in range(image.height):
            for x in range(image.width):
                bit = bool(data[y*((image.width+7)//8)+x//8] & (0x80>>(x%8)))
                assert bit == (image.getpixel((x,y)) == 0)
        all_rows.append({'name':name,'variant':label,'width':image.width,'height':image.height,'bytes':len(data)})
    lines += ['} // namespace cpw_assets', '#undef CPW_ASSET_FLASH', '']
    (assets/'include'/f'icons_{label}.h').write_text('\n'.join(lines),encoding='utf-8')

(assets/'include').mkdir(exist_ok=True)
for size in [16,24,32,48,64]:
    items=[]
    for file in sorted((assets/'raster'/str(size)).glob('*.png')):
        image=Image.open(file).convert('L').point(lambda v:255 if v>=160 else 0,mode='1')
        assert image.size==(size,size)
        bounds=ImageOps.invert(image.convert('L')).getbbox()
        assert bounds and bounds[0]>0 and bounds[1]>0 and bounds[2]<size and bounds[3]<size, (file,bounds)
        image.save(file)
        metadata[f'{file.stem}/{size}']={'width':size,'height':size,'ink_bounds':list(bounds)}
        items.append((file.stem,image))
    assert items
    emit_header(str(size),items)

current=[]
(assets/'current').mkdir(exist_ok=True)
for file in sorted((assets/'raster'/'64').glob('*.png')):
    if file.stem.startswith('ui-'): continue
    source=Image.open(file).convert('L')
    box=ImageOps.invert(source).getbbox()
    crop=source.crop(box)
    height=args.current_height
    width=round(crop.width*height/crop.height)
    resized=crop.resize((width,height),Image.Resampling.LANCZOS).point(lambda v:255 if v>=160 else 0,mode='1')
    # Match visible height exactly, independent of SVG canvas padding.
    ink=ImageOps.invert(resized.convert('L')).getbbox()
    assert ink and ink[1]==0 and ink[3]==height, (file,ink)
    canvas_width=max(96,8*math.ceil(width/8))
    image=Image.new('1',(canvas_width,height),1)
    image.paste(resized,((canvas_width-width)//2,0))
    image.save(assets/'current'/file.name)
    metadata[f'{file.stem}/current']={'width':canvas_width,'height':height,'ink_bounds':list(ImageOps.invert(image.convert('L')).getbbox())}
    current.append((file.stem,image))
emit_header('current',current)

# Contact sheet deliberately includes both actual firmware sizes and readable labels.
font_path=Path(os.environ.get('CPW_PREVIEW_FONT','C:/Windows/Fonts/arial.ttf'))
font=ImageFont.truetype(str(font_path),13)
small=ImageFont.truetype(str(font_path),11)
files=sorted((assets/'svg').glob('*.svg'))
sheet=Image.new('RGB',(1000,80+math.ceil(len(files)/5)*125),'white')
d=ImageDraw.Draw(sheet)
d.text((20,15),'CrowPanelWeather — original monochrome asset set',fill='black',font=ImageFont.truetype(str(font_path),22))
d.text((20,47),'64 px master   /   24 px forecast   /   16 px status',fill='black',font=font)
for i,file in enumerate(files):
    x=(i%5)*200+12;y=80+(i//5)*125
    d.text((x,y),file.stem,fill='black',font=small)
    for offset,size in [(0,64),(78,24),(120,16)]:
        img=Image.open(assets/'raster'/str(size)/(file.stem+'.png')).convert('RGB')
        sheet.paste(img,(x+offset,y+22))
sheet.save(assets/'preview.png')

# Actual-resolution integration previews; font is used for previews only, not redistributed.
for count in [3,5]:
    panel=Image.new('1',(400,300),1); draw=ImageDraw.Draw(panel)
    f=lambda n:ImageFont.truetype(str(font_path),n)
    draw.rectangle((0,0,399,27),fill=0)
    draw.text((10,7),'ZURICH',font=f(13),fill=1)
    draw.text((390,8),'TUE 27 OCT · 10:00',font=f(11),anchor='rt',fill=1)
    draw.text((12,36),'PARTLY CLOUDY',font=f(10),fill=0)
    draw.text((10,58),'14°',font=f(66),fill=0,anchor='lt')
    current_icon=Image.open(assets/'current'/'partly-cloudy-day.png')
    panel.paste(current_icon,(101,58))
    draw.text((12,119),'Feels 12°',font=f(14),fill=0)
    draw.text((12,141),'High 16°  Low 7°',font=f(14),fill=0)
    draw.text((12,163),'UV max 2',font=f(12),fill=0)
    draw.line((203,37,203,177),fill=0)
    draw.text((213,36),'TODAY · °C',font=f(10),fill=0)
    left,right,top,bottom=239,383,56,161
    yy=lambda temp:round(bottom-(temp-5)/15*(bottom-top))
    for tick in [5,10,15,20]:
        y=yy(tick);draw.text((left-6,y),str(tick),font=f(10),fill=0,anchor='rm')
        for x in range(left,right+1,4): draw.point((x,y),fill=0)
    vals=[8,8,7,7,8,10,12,14,16,16,15,13,11]
    draw.line([(round(left+i*(right-left)/12),yy(val))for i,val in enumerate(vals)],fill=0,width=2)
    for y in range(top,bottom,5):draw.line((round(left+(right-left)*10/24),y,round(left+(right-left)*10/24),min(y+2,bottom)),fill=0)
    for i,hour in enumerate(['00','06','12','18','24h']):draw.text((left+i*(right-left)/4,170),hour,font=f(9),fill=0,anchor='mt')
    draw.line((10,188,390,188),fill=0)
    forecast=[('Wed 28','partly-cloudy-day','15° / 8°','20%'),('Thu 29','rain','13° / 7°','75%'),('Fri 30','rain','12° / 6°','85%'),('Sat 31','partly-cloudy-day','14° / 7°','30%'),('Sun 01','clear-day','16° / 8°','10%')]
    for i,(day,name,temps,pop)in enumerate(forecast[:count]):
        x=10+(i+.5)*380/count
        if i:draw.line((round(10+i*380/count),198,round(10+i*380/count),277),fill=0)
        draw.text((x,197),day,font=f(11),anchor='mt',fill=0)
        panel.paste(Image.open(assets/'raster'/'24'/(name+'.png')),(round(x-12),214))
        draw.text((x,245),temps,font=f(13),anchor='mt',fill=0)
        draw.text((x,264),pop,font=f(11),anchor='mt',fill=0)
    draw.line((10,282,390,282),fill=0)
    draw.text((10,287),'Weather: Open-Meteo · Updated 10:00',font=f(9),fill=0)
    draw.text((390,287),'Hourly · Menu',font=f(9),fill=0,anchor='rt')
    panel.save(assets/f'classic-{count}.png')
(assets/'manifest.json').write_text(json.dumps({'format':'1-bpp; MSB first; row padded to bytes; black=1','current_visible_height':args.current_height,'assets':metadata,'bitmaps':all_rows},indent=2)+'\n',encoding='utf-8')
print(json.dumps({'original_icons':len(files),'exported_bitmaps':len(all_rows),'current_weather_icons':len(current),'validation':'All pixels round-tripped; square artwork unclipped; current ink height matched.'}))
