"""Run isolated production-component artwork probes on the K40; export known paths only."""
import argparse
import json
import re
import subprocess
import time
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'docs/evidence/p240/detail-artwork'
ADB = ['C:/Android/sdk/platform-tools/adb.exe', '-s', 'b6e950ee']


def install(apk):
    process = subprocess.Popen(ADB + ['install', '-r', '-t', str(apk)],
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    time.sleep(2)
    if process.poll() is None:
        subprocess.run(ADB + ['shell', 'uiautomator', 'dump', '/sdcard/kivo-install.xml'],
                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        raw = subprocess.check_output(ADB + ['exec-out', 'cat', '/sdcard/kivo-install.xml'])
        for node in ET.fromstring(raw).iter('node'):
            if node.get('text') in ('继续安装', '安装'):
                a, b, c, d = map(int, re.findall(r'\d+', node.get('bounds')))
                subprocess.check_call(ADB + ['shell', 'input', 'tap', str((a+c)//2), str((b+d)//2)])
                break
    output = process.communicate(timeout=90)[0].decode(errors='replace')
    assert process.returncode == 0 and 'Success' in output, output
    return output


def probe(label):
    assert re.fullmatch('[a-z0-9-]+', label)
    folder = OUT / label
    folder.mkdir(parents=True, exist_ok=True)
    process = subprocess.Popen(ADB + ['shell', 'am', 'instrument', '-w', '-e', 'label', label,
        'com.kivo.cinema.test/com.kivo.cinema.ui.detail.DetailArtworkInstrumentation'], stdout=subprocess.PIPE)
    # HyperOS may block the runner's background activity launch; explicitly foreground the host.
    time.sleep(3)
    subprocess.run(ADB + ['shell', 'am', 'start', '-n', 'com.kivo.cinema/.MainActivity'],
                   check=True, stdout=subprocess.DEVNULL)
    result = process.communicate(timeout=100)[0]
    (folder / 'instrumentation.txt').write_bytes(result)
    assert b'7 fixture captures saved' in result, result.decode(errors='replace')
    def export(name):
        data = subprocess.check_output(ADB + ['exec-out', 'run-as', 'com.kivo.cinema', 'cat',
            f'cache/detail-artwork-probe/{label}/{name}'])
        (folder / name).write_bytes(data)
        return data
    report = json.loads(export('report.json'))
    for case in report:
        export(case['case'] + '.png')
    print(f'{label}: {len(report)} cases exported')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('label')
    parser.add_argument('--install-test', action='store_true')
    parser.add_argument('--install-app', action='store_true')
    args = parser.parse_args()
    if args.install_app:
        print(install(ROOT / 'android/app/build/outputs/apk/debug/app-debug.apk'))
    if args.install_test:
        print(install(ROOT / 'android/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk'))
    probe(args.label)
