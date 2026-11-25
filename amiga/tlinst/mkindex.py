#!/usr/bin/env python3
"""Make the package index of the Amiga TeX Live installer.

usage: amiga/tlinst/mkindex.py texlive.tlpdb release url > tlinst.idx

texlive.tlpdb is the database of a frozen TeX Live year (tlnet-final of
the historic archive).  The index keeps only what the installer needs,
so that a 68k reads it in seconds instead of parsing 20 MB:

    # comment
    R <release> <repository URL>
    @ <name>
    c <category>             Package, Collection, Scheme, TLCore, ...
    r                        relocatable: unpack below texmf-dist
    s <size> <sha512>        of <name>.tar.xz
    d <dep> <dep> ...        dependencies (only those in the index)
    e <execute line>         one per line (addMap, AddHyphen, AddFormat)
    t <short description>

Left out: platform packages (name.platform: binaries for other
systems), documentation and source containers (separate files in the
archive, never loaded), and the programs we do not port (EXCLUDE).
"""
import sys

# Engines and tools without an Amiga port, and their collections.
# Dependencies on these are dropped.
EXCLUDE = {
    "collection-context", "context", "context-legacy",
    "collection-xetex", "xetex", "xetexconfig",
    "collection-texworks", "texworks",
    "collection-wintools",
    "asymptote", "biber", "biber-ms", "xindy", "tlshell",
}


def read(path):
    pkgs, cur = {}, None
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line:
                cur = None
                continue
            if line.startswith("name "):
                cur = {"name": line[5:], "dep": [], "exe": [], "cat": "",
                       "reloc": False, "size": 0, "sum": "", "desc": ""}
                pkgs[cur["name"]] = cur
            elif cur is None or line.startswith(" "):
                continue
            elif line.startswith("category "):
                cur["cat"] = line[9:]
            elif line.startswith("relocated 1"):
                cur["reloc"] = True
            elif line.startswith("depend "):
                cur["dep"].append(line[7:])
            elif line.startswith("execute "):
                cur["exe"].append(line[8:])
            elif line.startswith("containersize "):
                cur["size"] = int(line.split()[1])
            elif line.startswith("containerchecksum "):
                cur["sum"] = line.split()[1]
            elif line.startswith("shortdesc "):
                cur["desc"] = line[10:]
    return pkgs


def keep(p):
    n = p["name"]
    return ("." not in n and n not in EXCLUDE and not n.startswith("00texlive")
            and p["cat"] != "ConTeXt")


def main():
    tlpdb, release, url = sys.argv[1:4]
    pkgs = read(tlpdb)
    names = sorted(n for n, p in pkgs.items() if keep(p))
    kept = set(names)
    out = sys.stdout
    out.write("# tlinst index: TeX Live %s for AmigaOS, made from texlive.tlpdb\n" % release)
    out.write("R %s %s\n" % (release, url))
    for n in names:
        p = pkgs[n]
        out.write("@ %s\nc %s\n" % (n, p["cat"]))
        if p["reloc"]:
            out.write("r\n")
        # a package without files (most collections) has no container size
        if p["sum"]:
            out.write("s %d %s\n" % (p["size"], p["sum"]))
        deps = [d for d in p["dep"] if d in kept]
        if deps:
            out.write("d %s\n" % " ".join(deps))
        for e in p["exe"]:
            out.write("e %s\n" % e)
        if p["desc"]:
            out.write("t %s\n" % p["desc"])


if __name__ == "__main__":
    main()
