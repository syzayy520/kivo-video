#!/usr/bin/env python3
"""最小 WebDAV 测试服务器（221 真机验证用）。
支持：PROPFIND(Depth:0/1) + GET(Range) + HEAD + OPTIONS。
仅监听 0.0.0.0:8080，无认证——仅供局域网测试。"""
import http.server
import os
import urllib.parse
import re
import socketserver
import xml.etree.ElementTree as ET
from xml.sax.saxutils import escape

ROOT = r"C:\kivo-dav"
PORT = 8080

class DavHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def _path(self):
        p = urllib.parse.unquote(self.path)
        return os.path.join(ROOT, p.lstrip("/").replace("/", os.sep))

    def _is_dir(self, local):
        return os.path.isdir(local)

    def _send_xml(self, code, body):
        data = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", 'text/xml; charset="utf-8"')
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_PROPFIND(self):
        local = self._path()
        depth = self.headers.get("Depth", "0")
        if not os.path.exists(local):
            self.send_error(404)
            return
        entries = []
        if os.path.isdir(local) and depth == "1":
            for name in sorted(os.listdir(local)):
                fp = os.path.join(local, name)
                if os.path.isdir(fp):
                    entries.append((name, True, 0))
                else:
                    entries.append((name, False, os.path.getsize(fp)))
        elif os.path.isfile(local):
            entries.append((os.path.basename(local), False, os.path.getsize(local)))
        elif os.path.isdir(local):
            entries.append((os.path.basename(local) or local, True, 0))

        href = urllib.parse.quote(self.path)
        xml = '<?xml version="1.0" encoding="utf-8"?>\n<D:multistatus xmlns:D="DAV:">\n'
        # self
        is_dir = os.path.isdir(local) if os.path.exists(local) else True
        size = os.path.getsize(local) if os.path.isfile(local) else 0
        xml += f'<D:response><D:href>{escape(href)}</D:href><D:propstat><D:prop>'
        xml += f'<D:resourcetype><D:{"collection" if is_dir else "empty"}/></D:resourcetype>'
        if not is_dir:
            xml += f'<D:getcontentlength>{size}</D:getcontentlength>'
        xml += '</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>\n'
        # children
        for name, is_dir, size in entries:
            child_href = href.rstrip("/") + "/" + urllib.parse.quote(name)
            xml += f'<D:response><D:href>{escape(child_href)}</D:href><D:propstat><D:prop>'
            xml += f'<D:resourcetype><D:{"collection" if is_dir else "empty"}/></D:resourcetype>'
            if not is_dir:
                xml += f'<D:getcontentlength>{size}</D:getcontentlength>'
            xml += '</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>\n'
        xml += '</D:multistatus>\n'
        self._send_xml(207, xml)

    def _serve_range(self, local):
        if not os.path.isfile(local):
            self.send_error(404)
            return
        size = os.path.getsize(local)
        range_hdr = self.headers.get("Range", "")
        start, end = 0, size - 1
        is_range = False
        m = re.match(r"bytes=(\d+)-(\d*)", range_hdr)
        if m:
            is_range = True
            start = int(m.group(1))
            if m.group(2):
                end = min(int(m.group(2)), size - 1)
            else:
                end = size - 1
            if start >= size:
                self.send_response(416)
                self.send_header("Content-Range", f"bytes */{size}")
                self.end_headers()
                return
        total = end - start + 1
        self.send_response(206 if is_range else 200)
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(total))
        if is_range:
            self.send_header("Content-Range", f"bytes {start}-{end}/{size}")
        self.end_headers()
        if self.command == "HEAD":
            return
        with open(local, "rb") as f:
            f.seek(start)
            remaining = total
            while remaining > 0:
                chunk = f.read(min(64 * 1024, remaining))
                if not chunk:
                    break
                self.wfile.write(chunk)
                remaining -= len(chunk)

    def do_GET(self):
        self._serve_range(self._path())

    def do_HEAD(self):
        self._serve_range(self._path())

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Allow", "OPTIONS, GET, HEAD, PROPFIND")
        self.send_header("Content-Length", "0")
        self.end_headers()

class ThreadedServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True

if __name__ == "__main__":
    server = ThreadedServer(("0.0.0.0", PORT), DavHandler)
    print(f"WebDAV test server on 0.0.0.0:{PORT} root={ROOT}")
    server.serve_forever()
