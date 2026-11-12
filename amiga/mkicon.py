#!/usr/bin/env python3
"""mkicon.py: write the Workbench icon (.info) for the TeX-Shell script.

usage: mkicon.py OUTPUT.info

A project icon (WBPROJECT) whose default tool is IconX, which runs the
script with the icon's drawer as current directory.  The picture is
drawn here, 3 bit planes, in the colours of the MagicWB palette; only
colours 0-4 and 6 are used, so that it still looks right on a 4-colour
Workbench (where colour 4 shows as 0 and 6 as 2).
"""
import struct
import sys

W, H = 44, 36

# MagicWB colours: 0 grey (background), 1 black, 2 white, 3 blue,
# 4 dark grey, 6 beige
pix = [[0] * W for _ in range(H)]


def rect(x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            pix[y][x] = c


# page with folded corner, shadow
F = 7                              # size of the folded corner
rect(9, 3, 36, 33, 4)              # shadow (offset below)
rect(7, 1, 34, 31, 2)              # page
for y in range(1, 32):             # outline
    pix[y][7] = 1
    pix[y][34] = 1
for x in range(7, 35):
    pix[1][x] = 1
    pix[31][x] = 1
for j in range(F):                 # cut the corner: above the diagonal
    for x in range(34 - F + 1 + j, 37):
        pix[1 + j][x] = 0
    pix[1 + j][34 - F + 1 + j] = 1 # diagonal edge
for j in range(1, F):              # the fold: triangle below the diagonal
    for x in range(34 - F + 1, 34 - F + 1 + j):
        pix[1 + j][x] = 6
for j in range(F):
    pix[1 + j][34 - F + 1] = 1     # left edge of the fold
    pix[F][34 - F + 1 + j] = 1     # bottom edge of the fold
rect(35, 3, 36, F + 1, 0)          # no shadow next to the cut corner

# lines of "text"
for y in (24, 27):
    rect(10, y, 31, y, 4)

# the TeX logo in blue: T, lowered E, X (5x7 glyphs, 2x scaled width)
T = ["#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "]
E = ["#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"]
X = ["#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"]


def glyph(g, x0, y0):
    for j, row in enumerate(g):
        for i, ch in enumerate(row):
            if ch == "#":
                rect(x0 + i, y0 + 2 * j, x0 + i, y0 + 2 * j + 1, 3)


glyph(T, 11, 6)
glyph(E, 16, 9)
glyph(X, 22, 6)

# a small Shell window with a prompt, bottom right
rect(26, 21, 43, 34, 1)            # frame
rect(27, 22, 42, 23, 3)            # title bar
rect(27, 24, 42, 33, 0)            # inside
prompt = [
    "#     ",
    " #    ",
    "  #   ",
    " #    ",
    "#  ###",
]
for j, row in enumerate(prompt):
    for i, ch in enumerate(row):
        if ch == "#":
            pix[26 + j][29 + i] = 2

DEPTH = 3
words = (W + 15) // 16


def planes():
    out = b""
    for p in range(DEPTH):
        for y in range(H):
            row = 0
            for x in range(words * 16):
                bit = (pix[y][x] >> p) & 1 if x < W else 0
                row = (row << 1) | bit
            out += row.to_bytes(words * 2, "big")
    return out


def string(s):
    b = s.encode("latin-1") + b"\0"
    return struct.pack(">L", len(b)) + b


def main(out):
    tool = "C:IconX"
    tooltypes = ["WINDOW=NIL:", "DELAY=0"]
    gadget = struct.pack(
        ">LhhHHHHHLLLLLHL",
        0,              # NextGadget
        0, 0,           # LeftEdge, TopEdge
        W, H + 1,       # Width, Height
        4,              # Flags: GFLG_GADGIMAGE, highlight by complement
        3,              # Activation: RELVERIFY | GADGIMMEDIATE
        1,              # GadgetType: BOOLGADGET
        1,              # GadgetRender (non-NULL: image follows)
        0,              # SelectRender
        0, 0, 0,        # GadgetText, MutualExclude, SpecialInfo
        0,              # GadgetID
        1,              # UserData: WB_DISKREVISION 1 (OS 2.0+ icon)
    )
    head = struct.pack(">HH", 0xE310, 1) + gadget + struct.pack(
        ">BBLLllLLl",
        4, 0,           # do_Type WBPROJECT, pad
        1,              # do_DefaultTool (non-NULL)
        1,              # do_ToolTypes (non-NULL)
        -2147483648,    # do_CurrentX: NO_ICON_POSITION
        -2147483648,    # do_CurrentY
        0,              # do_DrawerData
        0,              # do_ToolWindow
        8192,           # do_StackSize
    )
    image = struct.pack(">hhhhhLBBL", 0, 0, W, H, DEPTH, 1, (1 << DEPTH) - 1, 0, 0)
    data = head + image + planes() + string(tool)
    data += struct.pack(">L", (len(tooltypes) + 1) * 4)
    for t in tooltypes:
        data += string(t)
    with open(out, "wb") as f:
        f.write(data)


if __name__ == "__main__":
    main(sys.argv[1])
