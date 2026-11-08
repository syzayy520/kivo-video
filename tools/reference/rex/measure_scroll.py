"""Measure media and foreground independently; never treat failed registration as zero motion.

Requires opencv-python-headless and numpy. Coordinates are full-resolution source pixels.
ROI constrains source features only, allowing foreground to move across the target image.
"""
import argparse
import hashlib
import json
from pathlib import Path

import cv2
import numpy as np


def register(source, target, roi):
    mask = np.zeros_like(source)
    x1, y1, x2, y2 = roi
    mask[y1:y2, x1:x2] = 255
    sift = cv2.SIFT_create(nfeatures=12000)
    ka, da = sift.detectAndCompute(source, mask)
    kb, db = sift.detectAndCompute(target, None)
    if da is None or db is None or len(db) < 2:
        return {"accepted": False, "reason": "insufficient descriptors"}
    pairs = cv2.BFMatcher().knnMatch(da, db, k=2)
    good = [m for m, n in pairs if m.distance < .7 * n.distance]
    if len(good) < 12:
        return {"accepted": False, "reason": "fewer than 12 matches", "matches": len(good)}
    a = np.float32([ka[m.queryIdx].pt for m in good])
    b = np.float32([kb[m.trainIdx].pt for m in good])
    cv2.setRNGSeed(7)
    matrix, inliers = cv2.estimateAffinePartial2D(a, b, method=cv2.RANSAC,
                                                ransacReprojThreshold=2.5, maxIters=5000)
    if matrix is None:
        return {"accepted": False, "reason": "RANSAC failed"}
    keep = inliers.ravel().astype(bool)
    count = int(keep.sum())
    ratio = count / len(good)
    prediction = a @ matrix[:, :2].T + matrix[:, 2]
    rmse = float(np.sqrt(np.mean(np.sum((prediction[keep] - b[keep]) ** 2, axis=1))))
    accepted = count >= 12 and ratio >= .35 and rmse <= 2.5
    return {"accepted": accepted, "reason": None if accepted else "low inlier count/ratio or excessive residual",
            "matches": len(good), "inliers": count, "inlier_ratio": ratio, "rmse_px": rmse,
            "scale": float(np.hypot(matrix[0, 0], matrix[1, 0])),
            "rotation_degrees": float(np.degrees(np.arctan2(matrix[1, 0], matrix[0, 0]))),
            "translation_px": matrix[:, 2].tolist(), "matrix": matrix.tolist()}


def read(path):
    data = path.read_bytes()
    gray = cv2.imdecode(np.frombuffer(data, dtype=np.uint8), cv2.IMREAD_GRAYSCALE)
    if gray is None:
        raise ValueError(f"Cannot decode {path}")
    return gray, {"path": str(path.resolve()), "sha256": hashlib.sha256(data).hexdigest(),
                  "size_px": [gray.shape[1], gray.shape[0]]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", type=Path)
    parser.add_argument("targets", nargs="+", type=Path)
    parser.add_argument("--media-roi", nargs=4, type=int, required=True)
    parser.add_argument("--foreground-roi", nargs=4, type=int, required=True)
    parser.add_argument("--pixels-per-unit", type=float, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    base, origin = read(args.base)
    rows = []
    for path in args.targets:
        target, identity = read(path)
        media = register(base, target, args.media_roi)
        foreground = register(base, target, args.foreground_roi)
        scroll = None
        if foreground["accepted"] and abs(foreground["scale"] - 1) < .015:
            # Displacement at the ROI centre discounts small numerical scale/rotation residuals.
            centre = np.array([(args.foreground_roi[0] + args.foreground_roi[2]) / 2,
                               (args.foreground_roi[1] + args.foreground_roi[3]) / 2, 1])
            scroll = float((centre[1] - (np.array(foreground["matrix"]) @ centre)[1]) / args.pixels_per_unit)
        rows.append({"image": identity, "media": media, "foreground": foreground, "foreground_up_units": scroll})
    result = {"opencv": cv2.__version__, "numpy": np.__version__, "base": origin,
              "media_roi": args.media_roi, "foreground_roi": args.foreground_roi,
              "pixels_per_unit": args.pixels_per_unit,
              "method": "SIFT ratio .7 + partial affine RANSAC 2.5px; >=12 inliers, ratio>=.35; RMSE<=2.5px",
              "rows": rows}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps([{ "file": r["image"]["path"], "media_accepted": r["media"]["accepted"],
                       "scale": r["media"].get("scale"), "foreground_up_units": r["foreground_up_units"]}
                      for r in rows], ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
