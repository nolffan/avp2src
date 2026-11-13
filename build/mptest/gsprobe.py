#!/usr/bin/env python3
"""One validated GameSpy list request to a master (TCP 28900).

  gsprobe.py <master host> <gamename> <secret key> [<port>]

Prints the master's challenge, what it answers and the servers it lists.
Sends only what the game's own browser sends (gamename, a version, the
validate response).
"""
import socket
import struct
import sys


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


def main():
    host, game, key = sys.argv[1:4]
    port = int(sys.argv[4]) if len(sys.argv) > 4 else 28900
    s = socket.create_connection((host, port), timeout=15)
    hello = s.recv(256)
    print("challenge:", hello)
    ch = hello.split(b"\\secure\\")[1].split(b"\\")[0][:6]
    req = "\\gamename\\%s\\gamever\\1.0\\location\\0\\validate\\%s\\final\\\\queryid\\1.1\\" % (game, validate(ch, key.encode()))
    s.sendall(req.encode())
    s.sendall(("\\list\\cmp\\gamename\\%s\\final\\" % game).encode())
    data = b""
    try:
        while True:
            d = s.recv(4096)
            if not d:
                break
            data += d
            if data.endswith(b"\\final\\"):
                break
    except socket.timeout:
        print("(timed out)")
    s.close()
    if data.startswith(b"\\error\\") or b"\\" in data[:1]:
        print("answer:", data[:300])
    body = data[:-7] if data.endswith(b"\\final\\") else data
    n = len(body) // 6
    print("bytes %d, servers %d" % (len(data), n))
    for i in range(n):
        ip = socket.inet_ntoa(body[i * 6:i * 6 + 4])
        (p,) = struct.unpack(">H", body[i * 6 + 4:i * 6 + 6])
        print("  %s:%d" % (ip, p))


main()
