"""Recompute the archived M1 static-media checks without modifying evidence."""
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image


def main():
    repo = Path(__file__).resolve().parents[3]
    root = repo / "docs/evidence/p240/media-motion"
    registration = json.loads((root / "android-registration.json").read_text(encoding="utf-8"))
    errors = []
    for row in registration["rows"]:
        path = root / "android" / Path(row["image"]["path"].replace("\\", "/")).name
        assert hashlib.sha256(path.read_bytes()).hexdigest() == row["image"]["sha256"], path
        assert row["media"]["accepted"] and row["foreground"]["accepted"], path
        assert row["foreground_up_units"] is not None, path
        model = 1 + min(350, max(0, row["foreground_up_units"])) / 2925
        errors.append(abs(row["media"]["scale"] - model))
        assert abs(row["foreground"]["scale"] - 1) < .001, path
    a = np.array(Image.open(root / "android/home-next-final.png").convert("RGB"), dtype=np.int16)[120:1800, :1080]
    b = np.array(Image.open(root / "android/home-cancel-final.png").convert("RGB"), dtype=np.int16)[120:1800, :1080]
    mae = float(np.abs(a - b).mean())
    assert max(errors) < .0002, errors
    assert mae < .5, mae
    off = json.loads((root / "preview-off-check.json").read_text(encoding="utf-8"))
    assert off["autoplay"] is False and off["new_events"] == 0, off
    manifest = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
    for item in manifest["files"]:
        data = (repo / item["path"]).read_bytes()
        if item["hash_mode"] == "lf-normalized":
            data = data.replace(b"\r\n", b"\n")
        assert hashlib.sha256(data).hexdigest() == item["sha256"], item["path"]
    print(json.dumps({"max_scale_error": max(errors), "cancel_rgb_mae": mae,
                      "manifest_files_verified": len(manifest["files"]),
                      "preview_new_events_off": off["new_events"]}, indent=2))


if __name__ == "__main__":
    main()
