"""Index Rex reference evidence without treating inferred timing as measured code."""

import argparse
import csv
import hashlib
import json
import re
import zipfile
from pathlib import Path


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_csv(path, rows, fields):
    with path.open("w", encoding="utf-8-sig", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    package = root / "docs/reference/rex-teardown/rex-package"
    output = package.parent / "audit"
    output.mkdir(parents=True, exist_ok=True)

    hashes = []
    for path in sorted(package.rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(package)
        original = None
        if args.source_root:
            original = args.source_root / relative
            if not original.is_file():
                original = args.source_root / "Rex_拆解成果_落地包" / relative
        entry = {"path": path.relative_to(root).as_posix(),
                 "sha256": sha256(path), "bytes": path.stat().st_size}
        entry["original_path"] = str(original) if original else None
        entry["original_sha256"] = sha256(original) if original and original.is_file() else None
        entry["matches_original"] = (entry["sha256"] == entry["original_sha256"]
                                     if entry["original_sha256"] else None)
        hashes.append(entry)
    (output / "source-hashes.json").write_text(
        json.dumps(hashes, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    csv_path = package / "Rex_动效参数表.csv"
    with csv_path.open(encoding="utf-8-sig", newline="") as source:
        reader = csv.DictReader(source)
        original_fields = reader.fieldnames
        interactions = list(reader)
    extra_fields = ["source_file", "runtime_status", "timing_status", "planned_scope"]
    for row in interactions:
        row["source_file"] = csv_path.relative_to(root).as_posix()
        row["runtime_status"] = "未逐条真机验证；原始等级仅保留，不继承为实测"
        row["timing_status"] = "推断/待测；文案或资源不证明时长和缓动"
        row["planned_scope"] = ("范围外：PRO" if "PRO" in row["模块"]
                                else "R5：播放器" if "播放器" in row["模块"]
                                else "R1盘点；按所属页面独立排卡")
    write_csv(output / "interactions.csv", interactions, original_fields + extra_fields)

    keywords = re.compile(
        r"滑|按|拖|预告|轮播|缩放|旋转|震|触觉|动效|动画|刷新|展开|收起|锁定|倍速|双击|孵|抽奖|"
        r"tap|press|swipe|drag|trailer|carousel|zoom|rotat|haptic|motion|animation|refresh|"
        r"expand|collapse|gesture|lock|speed", re.I)
    string_rows = []
    string_paths = sorted((package / "strings").glob("zh-Hans__*.json"))
    for path in string_paths:
        chinese = json.loads(path.read_text(encoding="utf-8-sig"))
        english_path = path.with_name(path.name.replace("zh-Hans__", "en__", 1))
        english = json.loads(english_path.read_text(encoding="utf-8-sig")) if english_path.exists() else {}
        for key, value in sorted(chinese.items()):
            translation = english.get(key, "")
            if keywords.search(f"{key} {value} {translation}"):
                string_rows.append({"source_file": path.relative_to(root).as_posix(),
                                    "key": key, "zh_Hans": value, "en": translation,
                                    "evidence_limit": "文案线索；入口/默认值/时序须真机确认"})
    write_csv(output / "string-evidence.csv", string_rows,
              ["source_file", "key", "zh_Hans", "en", "evidence_limit"])

    image_extensions = {".png", ".webp", ".jpg", ".jpeg", ".svg"}
    raw_images = [p.relative_to(package).as_posix() for p in package.rglob("*")
                  if p.is_file() and p.suffix.lower() in image_extensions]
    archives = {}
    for path in package.glob("*.zip"):
        with zipfile.ZipFile(path) as archive:
            names = archive.namelist()
            archives[path.name] = {"entries": len(names),
                                   "image_entries": [n for n in names if Path(n).suffix.lower() in image_extensions],
                                   "layers_entries": [n for n in names if Path(n).name == "rex-layers.json"]}
    (output / "archive-inventory.json").write_text(
        json.dumps(archives, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    matched = sum(e["matches_original"] is True for e in hashes)
    missing = sum(e["matches_original"] is None for e in hashes)
    mismatched = sum(e["matches_original"] is False for e in hashes)
    modules = sorted({r["模块"] for r in interactions})
    status = f"""# REA 拆解资料审计索引

生成工具：`tools/reference/rex/audit_package.py`。日期：2026-11-08。

## 实际清点

- 仓库包内文件 {len(hashes)} 份；原件哈希一致 {matched}，不一致 {mismatched}，原件未找到/未提供 {missing}。逐文件结果见 [source-hashes.json](source-hashes.json)。
- 动效 CSV 实际 {len(interactions)} 个场景（不计表头），模块：{'、'.join(modules)}。见 [interactions.csv](interactions.csv)。未逐条做运行时验证；原等级列原样保留。
- 文案 JSON {len(list((package / 'strings').glob('*.json')))} 份，中文模块 {len(string_paths)} 个；关键词筛出的中英文候选 {len(string_rows)} 条，见 [string-evidence.csv](string-evidence.csv)。这不是已验证交互总数，也不是所有交互的穷尽证明。
- 外层原始图像 {len(raw_images)} 份；ZIP 内清单见 [archive-inventory.json](archive-inventory.json)。`assets/*.json` 是资产元数据，不能直接作为图片使用。

## 哪些能用、怎样用

| 资料 | 可直接利用 | 必须再验证 |
| --- | --- | --- |
| 中英文 strings | 文案、无障碍语义、错误类别、待查能力线索 | 当前手机版入口、默认值、动作完整状态机 |
| assetutil 元数据 | 资产名、尺寸/色彩声明、图层或图像族线索 | 原始图层位图能否获取；运行时布局、裁切、混合方式 |
| 动效 CSV/拆解 MD | 场景索引、触发与取消条件候选、资源关联 | 时长、弹簧、延迟、位移、手势阈值与对象适用性 |
| 旧截图/录屏 | 已知版本/状态下的画面及有限时序 | 同素材、同设置对照与帧率误差 |
| 当前 USB 真机 | 可重现操作与最新版本结构 | 单张截图不证明动画曲线；按需连续录像 |

原始包标注 Rex 1.0.2，当前 iPhone Rex 1.1。FairPlay cryptid=1；不能声称已从加密 SwiftUI 代码提取精确参数。启动屏 393×852 也不证明全应用固定设计宽。

## 排期和缺口

首页/详情滚动、片名独立与下拉分层的最新截图见 `docs/evidence/p240/layered-scroll/`。本次没有把这些观察推广为所有 CSV 场景实测。

下一步 R1 优先核对详情操作和首页切页/刷新/预告取消，其余模块逐页登记；播放器单独 R5，PRO 不纳入。当前资料足够开始这些盘点，不需用户重录已有内容。只有 USB 无法重现的特定状态，才说明缺哪个场景、起止状态和录制要求。

新规划与旧结论处置见 `docs/design/ui/replica/roadmap.md` 和 `review.md`。复算命令：

```powershell
python tools/reference/rex/audit_package.py --source-root 'C:/Users/Administrator/Pictures/REX/rex/rex'
```
"""
    (output / "status.md").write_text(status, encoding="utf-8")
    print(json.dumps({"files": len(hashes), "matched": matched, "mismatched": mismatched,
                      "missing": missing, "scenes": len(interactions),
                      "string_candidates": len(string_rows), "raw_images": len(raw_images),
                      "archives": archives}, ensure_ascii=False))
    if mismatched:
        raise SystemExit("Reference copies differ from originals; inspect source-hashes.json")


if __name__ == "__main__":
    main()
