#!/usr/bin/env python3
# Rewrite hexadecimal float literals (0x1.8p+3) in a gzipped Lua table as
# decimal numbers.  The host's Lua writes luaotfload's font list with
# them; LuaTeX on the Amiga reads numbers with ixemul's strtod, which does
# not know hexadecimal floats ("malformed number near '0x0p+0'").
import gzip, re, sys
f = sys.argv[1]
s = gzip.open(f, "rt", encoding="latin-1").read()
pat = re.compile(r'(?<![\w."])(-?)0[xX]([0-9a-fA-F]*\.?[0-9a-fA-F]*)[pP]([+-]?\d+)')
def dec(m):
    v = float.fromhex(m.group(1) + "0x" + m.group(2) + "p" + m.group(3))
    return repr(int(v)) if v == int(v) and abs(v) < 2**53 else repr(v)
s, n = pat.subn(dec, s)
with gzip.open(f, "wt", encoding="latin-1") as g:
    g.write(s)
print(n, "numbers rewritten")
