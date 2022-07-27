#!/usr/bin/python3

import os

dirs = []

for entry in os.scandir('src'):
    if entry.is_dir():
        dirs.append(entry)

#print(dirs)