#!/usr/bin/env python3
"""230 测试件：最小 UPnP/DLNA MediaRenderer（PC 端）。

职责：给 K40 的投屏链路提供一个"可发现、可控制、真出画面"的渲染端——
  1) SSDP：周期发 ssdp:alive NOTIFY + 应答 M-SEARCH（ST: MediaRenderer）
  2) HTTP：设备描述 XML + AVTransport SOAP 控制端点（Set/Play/Pause/Stop/Seek）
  3) Set+Play 后用 ffplay 真渲染，全部动作追加进 tools/dlna_renderer.log（取证）

仅标准库。用法：python -X utf8 -I tools/dlna_renderer.py
停止：Ctrl+C（会发 ssdp:byebye）。
"""
import re
import socket
import subprocess
import threading
import time
import urllib.request
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

HOST_IP = "192.168.5.142"          # 本机 LAN IP（SSDP LOCATION 用）
HTTP_PORT = 8060
SSDP_UNICAST_PORT = 1901   # 测试件约定：Windows 对 1900 单播有 WFP 重定向（SSDPSRV 残留），1901 直达本进程
FFPLAY = r"C:/ffmpeg-sdk/ffmpeg-n7.1-latest-win64-gpl-shared-7.1/bin/ffplay.exe"
FFPROBE = (r"C:/ffmpeg-sdk/ffmpeg-n7.1-latest-win64-gpl-shared-7.1/"
           r"bin/ffprobe.exe")
LOG = Path(__file__).with_name("dlna_renderer.log")

FRIENDLY = "kivo-test-renderer"
AVT_TYPE = "urn:schemas-upnp-org:service:AVTransport:1"

state_lock = threading.Lock()
state = {"uri": None, "metadata": None, "ffplay": None, "actions": 0}


def log(msg: str) -> None:
    line = f"{datetime.now().strftime('%H:%M:%S.%f')[:-3]} {msg}"
    print(line, flush=True)
    with LOG.open("a", encoding="utf-8") as f:
        f.write(line + "\n")


# ---------------- ffplay 渲染 ----------------

def stop_ffplay() -> None:
    with state_lock:
        p = state.get("ffplay")
        state["ffplay"] = None
    if p and p.poll() is None:
        p.kill()
        log("ffplay killed")


def start_ffplay(uri: str) -> None:
    stop_ffplay()
    # 源可读性先行取证（能拿到的容器信息直接落日志）
    probe = ""
    try:
        r = subprocess.run([FFPROBE, "-v", "error", "-print_format", "json",
                            "-show_format", uri],
                           capture_output=True, timeout=20)
        probe = r.stdout.decode("utf-8", "replace")[:400]
    except Exception as e:
        probe = f"probe-failed: {e}"
    log(f"probe {uri[:120]} -> {probe[:200]}")
    with state_lock:
        state["ffplay"] = subprocess.Popen(
            [FFPLAY, "-x", "960", "-y", "540", "-autoexit", "-loglevel", "quiet", uri])
    log(f"ffplay started pid={state['ffplay'].pid} uri={uri}")


# ---------------- SSDP ----------------

def ssdp_alive_packet() -> bytes:
    lines = [
        "NOTIFY * HTTP/1.1",
        f"HOST: 239.255.255.250:1900",
        "CACHE-CONTROL: max-age=30",
        f"LOCATION: http://{HOST_IP}:{HTTP_PORT}/description.xml",
        f"NT: {AVT_TYPE}", f"NTS: ssdp:alive", f"SERVER: kivo-test/1.0",
        f"USN: uuid:kivo-test-renderer::{AVT_TYPE}",
        "", "",
    ]
    return "\r\n".join(lines).encode()


def answer_msearch(ss: socket.socket, addr) -> None:
    resp = [
        "HTTP/1.1 200 OK",
        "CACHE-CONTROL: max-age=30",
        f"DATE: {time.strftime('%a, %d %b %Y %H:%M:%S GMT')}",
        f"LOCATION: http://{HOST_IP}:{HTTP_PORT}/description.xml",
        f"ST: {AVT_TYPE}",
        f"USN: uuid:kivo-test-renderer::{AVT_TYPE}",
        "", "",
    ]
    try:
        ss.sendto("\r\n".join(resp).encode(), addr)
        log(f"M-SEARCH answered -> {addr[0]}:{addr[1]}")
    except OSError as e:
        log(f"answer send failed: {e}")


def unicast_loop(u: socket.socket, stop_evt: threading.Event) -> None:
    u.settimeout(0.5)
    while not stop_evt.is_set():
        try:
            data, addr = u.recvfrom(2048)
        except socket.timeout:
            continue
        except OSError:
            break
        if data.decode("utf-8", "replace").startswith("M-SEARCH"):
            answer_msearch(u, addr)


def ssdp_loop(stop_evt: threading.Event) -> None:
    ss = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    # 1901 单播 M-SEARCH 监听（与组播 socket 并行）
    try:
        u = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        u.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        u.bind(("", SSDP_UNICAST_PORT))
        threading.Thread(target=unicast_loop, args=(u, stop_evt), daemon=True).start()
        log(f"unicast m-search listener on :{SSDP_UNICAST_PORT}")
    except OSError as e:
        log(f"unicast listener failed: {e}")
    ss.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    ss.bind(("", 1900))
    # 组播出口绑到 LAN 网卡（不绑默认走第一块虚拟网卡，alive 出不了局域网）
    try:
        ss.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF,
                      socket.inet_aton(HOST_IP))
    except OSError as e:
        log(f"multicast if set failed: {e}")
    try:
        mreq = socket.inet_aton("239.255.255.250") + socket.inet_aton(HOST_IP)
        ss.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        log("multicast joined on " + HOST_IP)
    except OSError as e:
        log(f"multicast join failed: {e}")

    def announce():
        while not stop_evt.is_set():
            try:
                ss.sendto(ssdp_alive_packet(), ("239.255.255.250", 1900))
            except OSError as e:
                log(f"alive send failed: {e}")
            stop_evt.wait(3.0)

    threading.Thread(target=announce, daemon=True).start()

    rx = 0
    last_hb = time.time()
    while not stop_evt.is_set():
        try:
            ss.settimeout(0.5)
            data, addr = ss.recvfrom(2048)
            rx += 1
        except socket.timeout:
            pass
        except OSError as e:
            log(f"recv error: {e}")
            break
        if time.time() - last_hb >= 10:
            log(f"ssdp heartbeat: rx={rx}")
            rx = 0
            last_hb = time.time()
        if not data:
            continue
        text = data.decode("utf-8", "replace")
        if text.startswith("M-SEARCH"):
            st = re.search(r"ST:\s*(\S+)", text, re.I)
            st_val = st.group(1) if st else "ssdp:all"
            if st_val in ("ssdp:all", AVT_TYPE,
                          "upnp:rootdevice", "uuid:kivo-test-renderer"):
                answer_msearch(ss, addr)


# ---------------- HTTP: 描述 + SOAP ----------------

def description_xml() -> str:
    return f"""<?xml version="1.0" encoding="utf-8"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
 <specVersion><major>1</major><minor>0</minor></specVersion>
 <device>
  <deviceType>urn:schemas-upnp-org:device:MediaRenderer:1</deviceType>
  <friendlyName>{FRIENDLY}</friendlyName>
  <manufacturer>kivo-test</manufacturer>
  <modelName>kivo-renderer-1</modelName>
  <UDN>uuid:kivo-test-renderer</UDN>
  <serviceList>
   <service>
    <serviceType>{AVT_TYPE}</serviceType>
    <serviceId>urn:upnp-org:serviceId:AVTransport</serviceId>
    <SCPDURL>/scpd.xml</SCPDURL>
    <controlURL>/control/avtransport</controlURL>
    <eventSubURL>/event/avtransport</eventSubURL>
   </service>
  </serviceList>
 </device>
</root>"""


class Handler(BaseHTTPRequestHandler):

    def log_message(self, fmt, *args):   # 静默默认访问日志（自己记）
        pass

    def _reply(self, code: int, body: bytes, ctype: str) -> None:
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = urlsplit(self.path).path
        if path == "/description.xml":
            self._reply(200, description_xml().encode("utf-8"), "text/xml")
        elif path == "/scpd.xml":
            self._reply(200, b"<?xml version='1.0'?><scpd/>", "text/xml")
        elif path == "/event/avtransport":
            self._reply(200, b"", "text/xml")
        else:
            self._reply(404, b"not found", "text/plain")

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length).decode("utf-8", "replace")
        soap = self.headers.get("SOAPACTION", "")
        m = re.search(r"#(\w+)", soap or "")
        action = m.group(1) if m else "?"
        with state_lock:
            state["actions"] += 1
        log(f"SOAP {action} body={body[:220]}")

        uri = None
        if action == "SetAVTransportURI":
            mu = re.search(r"<CurrentURI>(<!\[CDATA\[)?([^<]+)", body)
            uri = mu.group(2).strip() if mu else None
            meta = re.search(r"<CurrentURIMetadata>(<!\[CDATA\[)?([^<]*)", body)
            with state_lock:
                state["uri"] = uri
                state["metadata"] = (meta.group(2)[:120] if meta else "")
            if uri is None:
                log("SetAVTransportURI: no CurrentURI parsed!")
        elif action == "Play":
            with state_lock:
                uri = state.get("uri")
            if uri:
                start_ffplay(uri)
            else:
                log("Play without SetAVTransportURI!")
        elif action == "Pause":
            log("Pause received (ffplay keeps last frame; noted)")
        elif action == "Stop":
            log("Stop received")
            stop_ffplay()
        elif action == "Seek":
            mt = re.search(r"<Target>([^<]+)</Target>", body)
            log(f"Seek target={mt.group(1) if mt else '?'} (ffplay 非交互，仅记录)")

        xml = ('<?xml version="1.0" encoding="utf-8"?>'
               '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" '
               's:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'
               f'<s:Body><u:{action}Response '
               f'xmlns:u="{AVT_TYPE}"></u:{action}Response></s:Body>'
               '</s:Envelope>')
        self._reply(200, xml.encode("utf-8"), "text/xml; charset=\"utf-8\"")


def main():
    log(f"renderer up: http://{HOST_IP}:{HTTP_PORT}/description.xml")
    stop_evt = threading.Event()
    threading.Thread(target=ssdp_loop, args=(stop_evt,), daemon=True).start()
    httpd = ThreadingHTTPServer((("0.0.0.0"), HTTP_PORT), Handler)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        stop_evt.set()
        stop_ffplay()
        log("renderer down")


if __name__ == "__main__":
    main()
