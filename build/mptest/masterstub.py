#!/usr/bin/env python3
"""A local GameSpy master server, for testing (M-15).

  masterstub.py <log> <gamename>=<key> [<gamename>=<key> ...]

UDP 27900: heartbeats. A server's first heartbeat is answered with
\\secure\\<challenge>; its \\validate\\ is checked against the game's key.
Every heartbeat is followed by a \\status\\ query, whose reply is logged.
\\statechanged\\2 removes the server.
TCP 28900: the list. \\basic\\\\secure\\<challenge>, then the client's
\\validate\\ is checked and \\list\\cmp\\gamename\\<game> answered with the
validated servers of that game (6 bytes each) and \\final\\.
Everything is logged, one line per event.
"""
import random
import select
import socket
import string
import sys
import time


def validate(challenge, key):
    st = list(range(256))
    j = 0
    for i in range(256):
        j = (j + st[i] + key[i % len(key)]) & 255
        st[i], st[j] = st[j], st[i]
    x = y = 0
    out = bytearray()
    for c in challenge:
        x = (c + x + 1) & 255
        y = (st[x] + y) & 255
        st[x], st[y] = st[y], st[x]
        out.append(c ^ st[(st[x] + st[y]) & 255])
    while len(out) % 3:
        out.append(0)
    dig = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
    res = bytearray()
    for i in range(0, len(out), 3):
        a, b, c = out[i:i + 3]
        res += bytes([dig[a >> 2], dig[((a & 3) << 4) | (b >> 4)], dig[((b & 15) << 2) | (c >> 6)], dig[c & 63]])
    return res.decode()


def kv(data):
    parts = data.decode("latin-1").split("\\")
    d = {}
    for i in range(1, len(parts) - 1, 2):
        d.setdefault(parts[i], parts[i + 1])
    return d


def chal():
    return "".join(random.choice(string.ascii_letters) for _ in range(6))


def main():
    logf = open(sys.argv[1], "a", buffering=1)
    keys = dict(a.split("=", 1) for a in sys.argv[2:])

    def log(*a):
        logf.write("%.3f %s\n" % (time.time(), " ".join(str(x) for x in a)))

    u = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    u.bind(("0.0.0.0", 27900))
    t = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    t.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    t.bind(("0.0.0.0", 28900))
    t.listen(5)
    servers = {}   # (ip, port) -> dict(game, challenge, ok)
    log("listening", keys)
    while True:
        r, _, _ = select.select([u, t], [], [], 1.0)
        if u in r:
            data, addr = u.recvfrom(4096)
            d = kv(data)
            log("udp from", addr, data[:300])
            if "heartbeat" in d:
                game = d.get("gamename", "")
                s = servers.get(addr)
                if d.get("statechanged") == "2":
                    servers.pop(addr, None)
                    log("server gone", addr, game)
                    continue
                if s is None or not s["ok"] or s["game"] != game:
                    c = chal()
                    servers[addr] = dict(game=game, challenge=c, ok=False)
                    u.sendto(("\\secure\\" + c).encode(), addr)
                    log("sent secure", c, "to", addr)
                u.sendto(b"\\status\\", addr)
            elif "validate" in d and addr in servers:
                s = servers[addr]
                want = validate(s["challenge"].encode(), keys.get(s["game"], "").encode())
                s["ok"] = d["validate"] == want
                log("server validate", addr, s["game"], "OK" if s["ok"] else "BAD (want %s)" % want)
            if "hostname" in d:
                log("status reply", addr, "hostname=%r mapname=%r numplayers=%r gametype=%r" % (
                    d.get("hostname"), d.get("mapname"), d.get("numplayers"), d.get("gametype")))
        if t in r:
            c, addr = t.accept()
            c.settimeout(5)
            ch = chal()
            c.sendall(("\\basic\\\\secure\\" + ch).encode())
            buf = b""
            try:
                while b"\\list\\" not in buf or not buf.endswith(b"\\final\\"):
                    d = c.recv(4096)
                    if not d:
                        break
                    buf += d
            except socket.timeout:
                pass
            log("tcp from", addr, buf[:300])
            d = kv(buf)
            game = d.get("gamename", "")
            ok = d.get("validate") == validate(ch.encode(), keys.get(game, "").encode())
            log("client validate", game, "OK" if ok else "BAD")
            if not ok:
                c.sendall(b"\\fatal\\1\\error\\Validation error\\final\\")
            else:
                lst = buf.split(b"\\list\\", 1)[1]
                lgame = lst.split(b"\\gamename\\", 1)[1].split(b"\\")[0].decode() if b"\\gamename\\" in lst else game
                out = b""
                for (ip, port), s in servers.items():
                    if s["game"] == lgame and s["ok"]:
                        out += socket.inet_aton(ip) + port.to_bytes(2, "big")
                c.sendall(out + b"\\final\\")
                log("listed", len(out) // 6, "servers of", lgame)
            c.close()


main()
