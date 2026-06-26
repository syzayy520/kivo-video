import subprocess, os
os.chdir(r'c:\kivo video')
subprocess.run(['cmake','--preset','vs-debug'],capture_output=True)
new_t=['cancel','timeout','collection','auth']
for t in new_t:
  prefix='kivo_video_provider_http_range_' if t not in ['collection','auth'] else 'kivo_video_provider_webdav_'
  tg=prefix+t+'_test'
  r=subprocess.run(['cmake','--build','build/vs-debug','--target',tg],capture_output=True)
  out=(r.stdout or b'')+(r.stderr or b''); text=out.decode('utf-8',errors='replace')
  if r.returncode!=0:
    for l in text.split('\n'):
      if 'error ' in l.lower(): print(l[:200]); break
    break
  exe=f'build/vs-debug/backend/Debug/{tg}.exe'
  if os.path.exists(exe):
    r2=subprocess.run([exe],capture_output=True)
    print(f'PASS {t}' if r2.returncode==0 else f'FAIL {t} exit={r2.returncode}')
# CTest
r3=subprocess.run(['ctest','--test-dir','build/vs-debug','-C','Debug','-R','provider_(http_range|webdav)_|kivo_evidence_json_validator','--output-on-failure'],capture_output=True,text=True)
for l in r3.stdout.split('\n'):
  if 'tests passed' in l or 'tests failed' in l: print(l)
