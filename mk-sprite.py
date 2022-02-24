#!/usr/bin/python

# genera un bytestream .c dalla pixmap RGBA8
# es.
# 0x23,0x12,0x56,0xAA,
# 0x23,0x12,0x56,0xAA,
# : : :

from PIL import Image
import sys
import re


cm = Image.open(sys.argv[1]).convert("RGBA")
name = re.sub(".*/","",sys.argv[1])
name = re.sub("[.].*","",name)
name = name.upper()
print(f"#define {name}_W {cm.width}")
print(f"#define {name}_H {cm.height}")
for y in range(cm.height):
    for x in range(cm.width):
        c = cm.getpixel((x,y))
        print("0x%02X,0x%02X,0x%02X,0x%02X,"%(c[0],c[1],c[2],c[3]))


"""

python mk-sprite.py png/play.png


"""

