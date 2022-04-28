# Resources

This folder contains the collection of assets used in the application, such as fonts, textures and external models, as well as our shaders. The sources are listed at the bottom of the page, including both royalty free and commercial licensed assets. Shaders, being the backbones of a graphics pipeline, is at the core of our OpenGL renderer. Our shaders are written for desktops with GLSL 4.6 support only (DSA, ILS, ATC, Anisotropic Filtering, Compute Shader), they will not work on other platforms.

## Icons font in ImGui

Currently we are using [Fork Awesome](https://forkaweso.me/Fork-Awesome/) 1.2 for rendering icons in ImGui, it is fully free and open source with a total of 796 icons available. You can find all icons at [this link](https://forkaweso.me/Fork-Awesome/icons/). Another nice option is the [Font Awesome](https://fontawesome.com/) library, which has become a paid product since Pro 5, the list of free icons are mostly solid so are not suitable for our GUI drawing at a font size of 18pt.

[Google material design](https://fonts.google.com/icons) icon font is the state-of-the-art alternative, but it is recently changing the codepoints for the icons, which still have some conflicts so are not yet stable. Keep an eye on the [updates](https://github.com/google/material-design-icons) ...

## Cubemap splitter

You can use the following script to convert a single cubemap (non-HDRI) into six cube faces.

<details>
<summary>View Code</summary>

```python3
#!/usr/bin/env python

from PIL import Image
import os, subprocess, sys

faces = {
    'posx': (2,1), 'posy': (1,0), 'posz': (1,1),
    'negx': (0,1), 'negy': (1,2), 'negz': (3,1)
}

path = os.path.abspath("") + "\\"

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
            print("Error: failed to split cubemap: ", filename)
            print("Usage: python cubemap_spliter.py skybox1.png skybox2.jpg")

```
</details>

<details>
<summary>View Code</summary>

```bash
python --version    # require python 3
pip install Pillow  # require PIL

cd downloads  # directory of your image file
python cubemap_spliter.py cubemap.png
```
</details>

Note that this script only works for cubemaps specified in the __standard layout__. (_source: [wikipedia](https://en.wikipedia.org/wiki/Cube_mapping)_)

<p align="center">
  <img src="https://upload.wikimedia.org/wikipedia/commons/e/ea/Cube_map.svg">
</p>

## Credits

- Scene 01 - [Cosmic Skybox](https://assetstore.unity.com/packages/2d/textures-materials/sky/skybox-series-free-103633) from Unity Asset Store
- Scene 01 - [Runestone Model](https://sketchfab.com/3d-models/runestone-1aa38beb0c3049a188edf2096a9731fb) from Sketchfab
- Scene 02 - [Field-Path HDRI](https://ihdri.com/interactive-hdri-skies/2021-June-HDRI-Skies/Field-Path-Steinbacher-Street-Georgensgm%c3%bcnd-Interactive/index.html) from iHDRI.com
- Scene 02 - [Brick Texture](https://ambientcg.com/view?id=Bricks072) from Ambient CG
- Scene 02 - [Marble Texture](https://ambientcg.com/view?id=Marble020) from Ambient CG
- Scene 02 - [Motorbike Model](https://www.cgtrader.com/free-3d-models/car/racing-car/bike1) from cgtrader
- Scene 03 - [Hotel Room HDRI](https://polyhaven.com/a/hotel_room) from Poly Haven
- Scene 03 - [SW500 Magnum Model](https://sketchfab.com/3d-models/smith-wesson-500-magnum-f7dbf63385644ab290b0f56428dcfde6) from Sketchfab
- Scene 03 - [Mandalorian Helmet](https://sketchfab.com/3d-models/the-mandalorian-helmet-1d606adf659849c794a85bca69045c4b) from Sketchfab
- Scene 04 - [Above The Clouds HDRI](https://www.3dart.it/en/download-free-hdri-cloud/) from HDRI-Hub
- Scene 04 - [Cloth Model](https://github.com/google/filament/tree/main/assets/models/cloth) from Google Filament
- Scene 04 - [Denim Normal Texture](https://3djungle.net/textures/denim/6600/) from 3D Jungle
- Scene 04 - [Fabric Texture](https://ambientcg.com/view?id=Fabric055) from Ambient CG
- Scene 05 - [Moonlit Sky HDRI](https://www.artstation.com/marketplace/p/wnXb/8k-night-sky-hdri-sequence) from ArtStation
- Scene 05 - [50 Noya Columns Model](https://www.artstation.com/marketplace/p/kxb9z/50-noya-column-basemeshes) from ArtStation
- Scene 05 - [猫耳鈴音 (3DMAX)](https://sketchfab.com/3d-models/nekomimi-suzune-2c7c7425701e457c9a5d4a161ca9d862) from Sketchfab
- Scene 05 - [猫耳鈴音 (VRChat)](https://3d.nicovideo.jp/works/td35906) from ニコニ立体 (Niconi Solid)

## Others / Tools

- [Azure Lane Laffey](https://sketchfab.com/3d-models/azure-lane-laffey-68b133ee14714483ae0ad19573142f09) avatar from Sketchfab
- Cornell Box, Chinese Dragon, Mori Knob and Lat-Long Sphere from [McGuire Computer Graphics Archive](https://www.casual-effects.com/data/index.html)
- 2022 New PBR Sponza scene from [Intel Graphics Research Samples Library](https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html)
- [25 Shader Ball Pack](https://flippednormals.com/downloads/25-shader-ball-pack/) by Hossein Hamidi
- MikuMikuDance Assets from [MMD-Downloads-Galore](https://www.deviantart.com/mmd-downloads-galore/gallery/)
- MikuMikuDance motion from [NatsumiSempai](https://www.deviantart.com/natsumisempai) on Deviant Art
- Character Rigging and Animation on [Mixamo](https://www.mixamo.com/#/)
- [pixiv](https://www.pixiv.net/) and [Booth](https://booth.pm/ja) marketplace (ピクシブ株式会社)
- [waifu2x](https://github.com/nagadomi/waifu2x) image super-resolution using deep convolutional neural networks
- [video2x](https://github.com/k4yt3x/video2x) video upscaling with GPU-accelerated machine learning
- [Waifu2x-Extension-GUI](https://github.com/neo-mashiro/Waifu2x-Extension-GUI) image and video super resolution
