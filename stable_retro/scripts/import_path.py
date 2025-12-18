#!/usr/bin/env python
import json
import os
import sys
import zipfile

import stable_retro.data


def _check_zipfile(f, process_f):
    with zipfile.ZipFile(f) as zf:
        for entry in zf.infolist():
            _root, ext = os.path.splitext(entry.filename)
            with zf.open(entry) as innerf:
                if ext == ".zip":
                    _check_zipfile(innerf, process_f)
                else:
                    process_f(entry.filename, innerf)


def main():
    paths = sys.argv[1:] or ["."]
    known_hashes = stable_retro.data.get_known_hashes()

    imported_games = 0

    def save_if_matches(filename, f):
        nonlocal imported_games
        try:
            data, hash = stable_retro.data.groom_rom(filename, f)
        except (OSError, ValueError):
            return
        if hash in known_hashes:
            game, ext, curpath = known_hashes[hash]
            print("Importing", game)
            game_path = os.path.join(curpath, game)
            rom_path = os.path.join(game_path, "rom%s" % ext)
            with open(rom_path, "wb") as f:
                f.write(data)

            metadata_path = os.path.join(game_path, "metadata.json")
            if os.path.exists(metadata_path):
                try:
                    with open(metadata_path) as mf:
                        metadata = json.load(mf)
                    original_name = metadata.get("original_rom_name")
                    if original_name:
                        with open(os.path.join(game_path, original_name), "wb") as of:
                            of.write(data)
                except (json.JSONDecodeError, OSError):
                    pass
            imported_games += 1

    for path in paths:
        for root, dirs, files in os.walk(path):
            for filename in files:
                filepath = os.path.join(root, filename)
                with open(filepath, "rb") as f:
                    _root, ext = os.path.splitext(filename)
                    if ext == ".zip":
                        # First, try to match the raw zip file's SHA-1 against known hashes
                        # Some datasets store the archive's SHA directly in rom.sha
                        save_if_matches(filename, f)
                        f.seek(0)
                        try:
                            _check_zipfile(f, save_if_matches)
                        except zipfile.BadZipFile:
                            pass
                    else:
                        save_if_matches(filename, f)

    print("Imported %i games" % imported_games)


if __name__ == "__main__":
    main()
