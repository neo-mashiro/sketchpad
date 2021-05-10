#!/usr/bin/env python

from PIL import Image
import os, subprocess, sys

path = os.path.abspath("") + "\\"

faces = {
    'posx': (2,1), 'posy': (1,2), 'posz': (1,1),
    'negx': (0,1), 'negy': (1,0), 'negz': (3,1)
}

def process_image(filename):
    extension = filename.split(".")[-1].lower()
    if extension not in ('jpg', 'png'):
        sys.exit("Error: image extension is not supported...")

    print("Info: loading image from disk:", os.path.join(path, filename), "...")

    img = Image.open(os.path.join(path, filename))
    size = img.size[0] / 4

    if (img.size[1] / 3) != size:
        sys.exit("Error: non-standard cubemap layout...")

    print("Info: image successfully loaded, face size =", size)

    for face, coord in faces.items():
        area = [coord[0], coord[1], coord[0] + 1, coord[1] + 1]
        area = [size * _ for _ in area]
        target = face + '.' + extension

        try:
            print("Info: converting face:", target, "...")
            square = img.crop(area)
            square.save(os.path.join(path, target))
        except:
            print("Error: failed to convert face:", target)


if __name__ == "__main__":
    for filename in sys.argv[1:]:
        try:
            process_image(filename)
            print("Convertion complete! :D")
        except:
            print("Error: failed to split cubemap:", filename)
            print("Usage: python cubemap_splitter.py 1.png 2.jpg")
