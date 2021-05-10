# sketchpad->lib

This folder contains some utility libraries and helper scripts.

## cubemap_splitter.py

Convert a single standard cubemap image to six cube faces and save them to the disk.
```bash
python --version    # require python 3
pip install Pillow  # require PIL

cd lib  # move cubemap to this folder
python cubemap_splitter.py skybox.png
```

## panorama_converter.py

Convert a 360° panoramic image to six cube faces and save them to the disk.
```bash
...
```
