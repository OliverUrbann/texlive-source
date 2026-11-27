-- tlinst.lua: install TeX Live packages on AmigaOS (and on the build host).
--
-- usage: texlua tlinst.lua [options] <command> [names...]
--
-- commands:
--   install <name>...   install packages, collections or schemes, with
--                       everything they need
--   remove <name>...    remove packages (not what they need)
--   list [what]         what: installed (default), schemes, collections,
--                       all
--   info <name>...      show a package and what it would bring along
--   bundle <file>...    unpack package bundles (TeXLive2025-*.tar.xz, made
--                       by mkbundles.sh) and configure: much faster than
--                       installing thousands of packages one by one
--   config              make the configuration again (font maps, hyphenation,
--                       file name database, formats, font list)
--
-- options:
--   --root DIR          the TeX drawer (default TeX:)
--   --repository URL    an http:// URL or a directory with archive/ in it
--                       (default: from the index, the frozen TeX Live year in
--                       the historic archive)
--   --proxy URL         HTTP proxy, e.g. http://proxy:8080
--   --dry-run           only show what would be done
--   --keep              keep downloaded containers (in tlpkg/download)
--   --no-formats        do not build formats
--   --no-fontlist       do not update LuaLaTeX's font list
--   --no-config         no configuration at all (for building bundles)
--   --list FILE         read more names from FILE, one per line (for
--                       install also several on a line): AmigaDOS command
--                       lines are limited to about 500 characters
--   --tlunpack PROG     the tlunpack program (default: in bin/m68k-amigaos)
--   --mktexlsr PROG     the mktexlsr program (default: in bin/m68k-amigaos;
--                       "lua" uses the slower built-in version)
--
-- The package index (tlpkg/tlinst.idx) is made from texlive.tlpdb by
-- mkindex.py.  The TeX Live year is fixed: the programs of this port and
-- the packages must belong together.
--
-- Return code: 0 ok, 10 error (AmigaDOS ERROR).

local here = (arg and arg[0] or ""):match("^(.*[/:])") or ""
package.path = here .. "?.lua;" .. package.path
local net = require("tlnet")
local lfs = require("lfs")

local VERSION = "0.1"

-- ---------------------------------------------------------------------
-- small helpers

local function J(a, b)                 -- join paths, Amiga or Unix style
  if a == "" then return b end
  local last = a:sub(-1)
  if last == ":" or last == "/" then return a .. b end
  return a .. "/" .. b
end

local function exists(p) return lfs.attributes(p, "mode") ~= nil end
local function isdir(p) return lfs.attributes(p, "mode") == "directory" end

local function mkdirs(p)               -- create p and its parents
  if p == "" or isdir(p) then return true end
  local parent = p:match("^(.*)[/]") or p:match("^(.*:)") or ""
  if parent ~= "" and parent ~= p and not parent:match(":$") then
    mkdirs(parent)
  end
  if not isdir(p) then lfs.mkdir(p) end
  return isdir(p)
end

-- on stdout as well: started from the Installer, only stdout has a
-- window
local function die(msg)
  io.stdout:write("tlinst: ", msg, "\n")
  io.stdout:flush()
  os.exit(10)
end

local function say(...) io.write(...); io.write("\n"); io.flush() end

local function mb(n) return string.format("%.1f MB", n / 1048576) end

local function quote(s) return '"' .. s .. '"' end

local function run(cmd)                -- true if the command returned 0
  local ok, how, code = os.execute(cmd)
  if type(ok) == "number" then return ok == 0, ok end   -- Lua 5.1 style
  return ok == true and (code == nil or code == 0), code
end

local function readlines(p)
  local t, f = {}, io.open(p, "r")
  if not f then return nil end
  for l in f:lines() do t[#t + 1] = l end
  f:close()
  return t
end

local function writefile(p, s)
  local f, err = io.open(p, "w")
  if not f then die("cannot write " .. p .. ": " .. tostring(err)) end
  f:write(s)
  f:close()
end

-- ---------------------------------------------------------------------
-- options

local opt = { root = "TeX:", proxy = nil, dry = false, keep = false,
              formats = true, fontlist = true, verbose = false }
local cmd, names = nil, {}
do
  local i = 1
  while arg[i] do
    local a = arg[i]
    local function val() i = i + 1; return arg[i] or die("missing value for " .. a) end
    if a == "--root" then opt.root = val()
    elseif a == "--repository" then opt.repo = val()
    elseif a == "--proxy" then opt.proxy = val()
    elseif a == "--list" then opt.list = val()
    elseif a == "--tlunpack" then opt.tlunpack = val()
    elseif a == "--mktexlsr" then opt.mktexlsr = val()
    elseif a == "--dry-run" then opt.dry = true
    elseif a == "--keep" then opt.keep = true
    elseif a == "--verbose" then opt.verbose = true
    elseif a == "--no-formats" then opt.formats = false
    elseif a == "--no-fontlist" then opt.fontlist = false
    elseif a == "--no-config" then opt.noconfig = true
    elseif a == "--version" then say("tlinst ", VERSION); os.exit(0)
    elseif a:sub(1, 2) == "--" then die("unknown option " .. a)
    elseif not cmd then cmd = a
    else names[#names + 1] = a end
    i = i + 1
  end
end
if opt.list then
  local f = io.open(opt.list, "r") or die("cannot read " .. opt.list)
  for l in f:lines() do
    l = l:match("^%s*(.-)%s*$")
    if cmd == "bundle" then                     -- file names may have spaces
      if l ~= "" then names[#names + 1] = l end
    else
      for n in l:gmatch("%S+") do names[#names + 1] = n end
    end
  end
  f:close()
end
if not cmd then
  say("usage: tlinst [options] install|remove|bundle|list|info|config [names...]")
  say("see the comment at the top of tlinst.lua for the options")
  os.exit(10)
end

local R = opt.root
local BIN = J(R, "bin/m68k-amigaos")
local TLPKG = J(R, "tlpkg")
local DIST = J(R, "texmf-dist")
local VAR = J(R, "texmf-var")
opt.tlunpack = opt.tlunpack or J(BIN, "tlunpack")
if not opt.mktexlsr then
  opt.mktexlsr = exists(J(BIN, "mktexlsr")) and J(BIN, "mktexlsr") or "lua"
end

-- ---------------------------------------------------------------------
-- the index

local idx, release, defrepo = {}, nil, nil
do
  local f = io.open(J(TLPKG, "tlinst.idx"), "r")
  if not f then die("no package index " .. J(TLPKG, "tlinst.idx")) end
  local p
  for l in f:lines() do
    local k, v = l:sub(1, 1), l:sub(3)
    if k == "@" then
      p = { name = v, deps = {}, exe = {}, size = 0 }
      idx[v] = p
    elseif k == "c" then p.cat = v
    elseif k == "r" then p.reloc = true
    elseif k == "s" then
      local s, sum = v:match("^(%d+) (%x+)")
      p.size, p.sum = tonumber(s), sum
    elseif k == "d" then
      for d in v:gmatch("%S+") do p.deps[#p.deps + 1] = d end
    elseif k == "e" then p.exe[#p.exe + 1] = v
    elseif k == "t" then p.desc = v
    elseif k == "R" then release, defrepo = v:match("^(%S+) (%S+)")
    end
  end
  f:close()
end
opt.repo = opt.repo or defrepo

-- installed packages: tlpkg/installed.dat ("name sha512" per line) for
-- packages installed one by one, tlpkg/bundles/*.dat for those that came
-- with a bundle
local installed = {}
local INSTDB = J(TLPKG, "installed.dat")
local BUNDLES = J(TLPKG, "bundles")
local function read_installed()
  local t = {}
  local files = {}
  if isdir(BUNDLES) then
    for e in lfs.dir(BUNDLES) do
      if e:match("%.dat$") then files[#files + 1] = J(BUNDLES, e) end
    end
    table.sort(files)
  end
  files[#files + 1] = INSTDB
  for _, file in ipairs(files) do
    for _, l in ipairs(readlines(file) or {}) do
      local n, s = l:match("^(%S+)%s*(%S*)")
      if n then t[n] = s end
    end
  end
  return t
end
installed = read_installed()
-- the whole list, sorted (at the end of a run)
local function save_installed()
  local t = {}
  for n, s in pairs(installed) do t[#t + 1] = n .. " " .. s end
  table.sort(t)
  writefile(INSTDB, table.concat(t, "\n") .. "\n")
end
-- one more package (after each one, so that a broken-off run can go on;
-- a later line for the same name wins when the file is read)
local function add_installed(n, s)
  installed[n] = s
  local f = io.open(INSTDB, "a")
  if f then f:write(n, " ", s, "\n"); f:close() end
end
local LISTS = J(TLPKG, "lists")
local function listfile(n) return J(LISTS, n .. ".list") end

-- names and everything they depend on, in a stable order
local function closure(list)
  local seen, order = {}, {}
  local function add(n)
    if seen[n] then return end
    seen[n] = true
    local p = idx[n]
    if not p then return end
    for _, d in ipairs(p.deps) do add(d) end
    order[#order + 1] = n
  end
  for _, n in ipairs(list) do
    if not idx[n] then die("no package " .. n .. " in TeX Live " .. tostring(release)) end
    add(n)
  end
  return order
end

-- ---------------------------------------------------------------------
-- install

local is_url = opt.repo:match("^https?://") ~= nil

local function fetch(p, dlfile)
  local rel = "archive/" .. p.name .. ".tar.xz"
  if not is_url then return J(opt.repo, rel), false end
  local last = -1
  local ok, err = net.fetch(opt.repo .. "/" .. rel, dlfile, {
    size = p.size, proxy = opt.proxy,
    progress = function(got, size)
      if size and size > 1048576 then
        local pct = math.floor(got * 100 / size)
        if pct >= last + 10 then last = pct; io.write(" ", pct, "%"); io.flush() end
      end
    end,
    log = function(m) io.write("\n    ", m) end })
  if not ok then return nil, err end
  return dlfile, true
end

-- formats depend on these (and on any hyphenation package)
local format_pkgs = {}
for _, n in ipairs(closure({ "collection-basic", "collection-latex" })) do
  format_pkgs[n] = true
end

local changed = { formats = false, fonts = false, maps = false, any = false }

local function note_changes(p, files)
  changed.any = true
  if format_pkgs[p.name] then changed.formats = true end
  for _, e in ipairs(p.exe) do
    if e:match("^AddHyphen") or e:match("^AddFormat") then changed.formats = true end
    if e:match("^add%a*Map") then changed.maps = true end
  end
  for _, f in ipairs(files or {}) do
    if f:match("^fonts/opentype/") or f:match("^fonts/truetype/") then
      changed.fonts = true
      break
    end
  end
end

local function install(list)
  local todo, total = {}, 0
  for _, n in ipairs(closure(list)) do
    local p = idx[n]
    if installed[n] == nil or (p.sum and installed[n] ~= p.sum) then
      todo[#todo + 1] = p
      total = total + p.size
    end
  end
  if #todo == 0 then say("Everything is installed already."); return end
  say(string.format("%d packages to install, %s to %s.", #todo, mb(total),
                    is_url and "download" or "unpack"))
  if opt.dry then
    for _, p in ipairs(todo) do say("  ", p.name, "  ", mb(p.size)) end
    return
  end
  mkdirs(LISTS)
  local dldir = J(TLPKG, "download")
  mkdirs(dldir)
  local failed = {}
  for i, p in ipairs(todo) do
    io.write(string.format("[%d/%d] %s", i, #todo, p.name))
    if p.size > 0 then io.write(" (", mb(p.size), ")") end
    io.flush()
    if not p.sum then                  -- a collection or scheme: no files
      add_installed(p.name, "")
      say("")
    else
      local socket = require("socket")
      local t0 = socket.gettime()
      local file, err = fetch(p, J(dldir, p.name .. ".tar.xz"))
      local t1 = socket.gettime()
      if not file then
        say("\n    FAILED: ", err)
        failed[#failed + 1] = p.name
      else
        local dest = p.reloc and DIST or R
        local lst = listfile(p.name)
        local ok = run(quote(opt.tlunpack) .. " -c " .. p.sum .. " -x tlpkg/ -l " ..
                       quote(lst) .. " -d " .. quote(dest) .. " " .. quote(file) ..
                       " >" .. quote(J(dldir, "tlunpack.log")))
        local t2 = socket.gettime()
        if err and not opt.keep then os.remove(file) end      -- err: downloaded
        if ok then
          local files = readlines(lst) or {}
          -- non-relocatable packages list paths from the root
          if not p.reloc then
            local t = {}
            for _, f in ipairs(files) do t[#t + 1] = (f:gsub("^texmf%-dist/", "")) end
            files = t
          end
          add_installed(p.name, p.sum)
          note_changes(p, files)
          if opt.verbose then
            say(string.format("  ok  (get %.2fs, unpack %.2fs, rest %.2fs)", t1 - t0, t2 - t1,
                              socket.gettime() - t2))
          else
            say("  ok")
          end
        else
          say("\n    FAILED: damaged download or disk full (see ", J(dldir, "tlunpack.log"), ")")
          failed[#failed + 1] = p.name
        end
      end
    end
  end
  save_installed()
  if #failed > 0 then
    say("Not installed: ", table.concat(failed, " "))
    say("Run the same command again to try them again.")
  end
  return #failed == 0
end

local function remove(list)
  for _, n in ipairs(list) do
    if installed[n] == nil then
      say(n, " is not installed")
    else
      local p = idx[n] or { name = n, exe = {}, reloc = true }
      local base = p.reloc and DIST or R
      local files = readlines(listfile(n)) or {}
      if not opt.dry then
        for _, f in ipairs(files) do os.remove(J(base, f)) end
        os.remove(listfile(n))
        installed[n] = nil
        note_changes(p, files)
        -- take it out of the bundle lists too
        if isdir(BUNDLES) then
          for e in lfs.dir(BUNDLES) do
            if e:match("%.dat$") then
              local t, hit = {}, false
              for _, l in ipairs(readlines(J(BUNDLES, e)) or {}) do
                if l:match("^(%S+)") == n then hit = true else t[#t + 1] = l end
              end
              if hit then writefile(J(BUNDLES, e), table.concat(t, "\n") .. "\n") end
            end
          end
        end
      end
      say("removed ", n, " (", #files, " files)")
    end
  end
  if not opt.dry then save_installed() end
end

-- unpack bundles: one tlunpack run per bundle; the bundle brings the file
-- lists (tlpkg/lists) and its package list (tlpkg/bundles/<name>.dat)
local function bundles(list)
  local ok = true
  for i, file in ipairs(list) do
    if not exists(file) then
      say("no file ", file); ok = false
    else
      say(string.format("[%d/%d] %s (%s), unpacking ...", i, #list, file,
                        mb(lfs.attributes(file, "size") or 0)))
      if opt.dry then
        say("  (dry run)")
      elseif run(quote(opt.tlunpack) .. " -p -d " .. quote(R) .. " " .. quote(file)) then
        say("  ok")
      else
        say("  FAILED: damaged file or disk full")
        ok = false
      end
    end
  end
  if opt.dry then return ok end
  -- what came new, for the configuration
  local before = installed
  installed = read_installed()
  for n, s in pairs(installed) do
    if before[n] ~= s and idx[n] then
      local p = idx[n]
      local files = readlines(listfile(n)) or {}
      if not p.reloc then
        local t = {}
        for _, f in ipairs(files) do t[#t + 1] = (f:gsub("^texmf%-dist/", "")) end
        files = t
      end
      note_changes(p, files)
    end
  end
  return ok
end

-- ---------------------------------------------------------------------
-- configuration: what tlmgr and updmap do on Unix

-- all files of installed packages: name -> path below texmf-dist
local function file_table(pattern)
  local t = {}
  for n in pairs(installed) do
    local p = idx[n]
    local reloc = not p or p.reloc
    for _, f in ipairs(readlines(listfile(n)) or {}) do
      if not reloc then f = f:gsub("^texmf%-dist/", "") end
      local base = f:match("([^/]+)$")
      if base and base:match(pattern) then t[base] = f end
    end
  end
  return t
end

-- execute lines of installed packages, in package name order
local function executes()
  local ns = {}
  for n in pairs(installed) do if idx[n] then ns[#ns + 1] = n end end
  table.sort(ns)
  local t = {}
  for _, n in ipairs(ns) do
    for _, e in ipairs(idx[n].exe) do t[#t + 1] = { pkg = n, line = e } end
  end
  return t
end

local function read_head(name)        -- a header file of hyphen-base
  local f = io.open(J(DIST, "tex/generic/config/" .. name), "r")
  if not f then return "" end
  local s = f:read("a")
  f:close()
  return s
end

-- language.dat, language.def, language.dat.lua (TeXLive::TLPOBJ)
local function make_languages()
  local dat, def, lua = {}, {}, {}
  local last
  for _, e in ipairs(executes()) do
    local a = e.line:match("^AddHyphen%s+(.*)")
    if a then
      local r = { synonyms = {}, databases = { dat = true, def = true, lua = true } }
      for k, v in a:gmatch("([%w_]+)=(%S*)") do
        if k == "synonyms" or k == "databases" then
          local t = {}
          for x in v:gmatch("[^,]+") do t[#t + 1] = x end
          if k == "synonyms" then r.synonyms = t
          else r.databases = {}; for _, x in ipairs(t) do r.databases[x] = true end end
        else
          r[k] = v
        end
      end
      if e.pkg ~= last then
        dat[#dat + 1] = "% from " .. e.pkg .. ":"
        def[#def + 1] = "% from " .. e.pkg .. ":"
        lua[#lua + 1] = "-- from " .. e.pkg .. ":"
        last = e.pkg
      end
      if r.comment then
        dat[#dat + 1] = "% " .. r.comment
        def[#def + 1] = "% " .. r.comment
        lua[#lua + 1] = "-- " .. r.comment
      end
      local lhm, rhm = r.lefthyphenmin, r.righthyphenmin
      if r.databases.dat then
        dat[#dat + 1] = r.name .. " " .. r.file
        for _, s in ipairs(r.synonyms) do dat[#dat + 1] = "=" .. s end
      end
      if r.databases.def then
        def[#def + 1] = string.format("\\addlanguage{%s}{%s}{}{%s}{%s}", r.name, r.file, lhm, rhm)
        for _, s in ipairs(r.synonyms) do
          def[#def + 1] = string.format("\\addlanguage{%s}{%s}{}{%s}{%s}", s, r.file, lhm, rhm)
        end
      end
      if r.databases.lua then
        local syn = {}
        for _, s in ipairs(r.synonyms) do syn[#syn + 1] = "'" .. s .. "'" end
        local t = { "['" .. r.name .. "'] = {", "\tloader = '" .. r.file .. "',",
                    "\tlefthyphenmin = " .. lhm .. ",", "\trighthyphenmin = " .. rhm .. ",",
                    "\tsynonyms = { " .. table.concat(syn, ", ") .. " }," }
        if r.file_patterns then t[#t + 1] = "\tpatterns = '" .. r.file_patterns .. "'," end
        if r.file_exceptions then t[#t + 1] = "\thyphenation = '" .. r.file_exceptions .. "'," end
        if r.luaspecial then t[#t + 1] = "\tspecial = '" .. r.luaspecial .. "'," end
        t[#t + 1] = "},"
        for _, x in ipairs(t) do lua[#lua + 1] = "\t" .. x end
      end
    end
  end
  local dir = J(VAR, "tex/generic/config")
  mkdirs(dir)
  local gen = "Generated by tlinst (TeX Live for AmigaOS)"
  writefile(J(dir, "language.dat"), "% " .. gen .. "\n" .. read_head("language.us") ..
            table.concat(dat, "\n") .. "\n")
  -- language.us.def ends with nothing to keep; its first line must stay first
  writefile(J(dir, "language.def"), read_head("language.us.def") .. table.concat(def, "\n") ..
            "\n%%% No changes may be made beyond this point.\n\n" ..
            "\\uselanguage {USenglish}             %%% This MUST be the last line of the file.\n")
  writefile(J(dir, "language.dat.lua"), "-- " .. gen .. "\n" .. read_head("language.us.lua") ..
            table.concat(lua, "\n") .. "\n}\n")
end

-- pdftex.map: updmap's pdftex_dl14.map with the default settings
-- (LW35=URWkb, pdftexDownloadBase14=true)
local function make_pdftex_map()
  local mapfiles = file_table("%.map$")
  local function read_map(name)
    local path = mapfiles[name]
    if not path then return nil end
    local t = {}
    local f = io.open(J(DIST, path), "r")
    if not f then return nil end
    for l in f:lines() do
      if not l:match("^%s*[#%%]") and not l:match("^%s*$") then
        local a, b = l:match("^%s*(%S+)%s+(.-)%s*$")
        if a then t[a] = b or "" end
      end
    end
    f:close()
    return t
  end
  local maps, kinds = {}, {}
  for _, e in ipairs(executes()) do
    local kind, m = e.line:match("^(add%a*Map)%s+(%S+)")
    if kind == "addMap" or kind == "addMixedMap" then
      maps[#maps + 1] = m
      kinds[m] = kind
    end
  end
  table.sort(maps)
  -- a font defined in several maps: the last one read wins (updmap:
  -- "arbitrary instance"); the base 35 fonts can be replaced by any map
  local origin, content, missing = {}, {}, {}
  local base = read_map("ps2pk35.map")
  if not base then return say("  no ps2pk35.map: no pdftex.map made") end
  for font in pairs(base) do origin[font] = "ps2pk35.map" end
  for _, m in ipairs(maps) do
    local c = read_map(m)
    if c then
      content[m] = c
      for font in pairs(c) do origin[font] = m end
    else
      missing[#missing + 1] = m
    end
  end
  local lines, seen = {}, {}
  local function add(l)
    l = l:gsub("%s+", " "):gsub("%s$", "")
    -- as updmap: its second substitution is a no-op (Perl /x), so the
    -- quotes keep a space on both sides
    l = l:gsub('%s*"%s*', ' " ')
    if l ~= "" and not seen[l] then seen[l] = true; lines[#lines + 1] = l end
  end
  local function emit(name, c, all)
    add("% " .. name)
    local fonts = {}
    for font in pairs(c) do
      if all or origin[font] == name then fonts[#fonts + 1] = font end
    end
    table.sort(fonts)
    for _, font in ipairs(fonts) do add(font .. " " .. c[font]) end
  end
  add("% ps2pk35.map")
  emit("ps2pk35.map", base, true)
  for _, kind in ipairs({ "addMixedMap", "addMap" }) do
    for _, m in ipairs(maps) do
      if kinds[m] == kind and content[m] then emit(m, content[m], false) end
    end
  end
  local dir = J(VAR, "fonts/map/pdftex/updmap")
  mkdirs(dir)
  writefile(J(dir, "pdftex.map"), table.concat(lines, "\n") .. "\n")
  if #missing > 0 then say("  map files not found: ", table.concat(missing, " ")) end
end

-- ls-R, the same format as amiga/mktexlsr.c and mkpackage.sh write
local function lsr_lua(dir)
  local out = { "% ls-R -- filename database for kpathsea; do not change this line.", "" }
  local function walk(rel)
    local full = rel == "" and dir or J(dir, rel)
    local files, dirs = {}, {}
    for e in lfs.dir(full) do
      if e ~= "." and e ~= ".." and not e:match("%.uaem$") then
        files[#files + 1] = e
        if isdir(J(full, e)) then dirs[#dirs + 1] = e end
      end
    end
    table.sort(files)
    table.sort(dirs)
    out[#out + 1] = (rel == "" and "./" or "./" .. rel) .. ":"
    for _, e in ipairs(files) do out[#out + 1] = e end
    out[#out + 1] = ""
    for _, d in ipairs(dirs) do walk(rel == "" and d or rel .. "/" .. d) end
  end
  walk("")
  writefile(J(dir, "ls-R"), table.concat(out, "\n") .. "\n")
end

local function mktexlsr(dir)
  if not isdir(dir) then return end
  if opt.mktexlsr == "lua" then lsr_lua(dir)
  else run(quote(opt.mktexlsr) .. " " .. quote(dir)) end
end

-- pdflatex and lualatex are pdftex and luahbtex under another name (the
-- name picks the format).  AmigaOS 3 has no usable links: copy them, with
-- the AmigaDOS Copy, which keeps the protection bits.  Not in the archive,
-- to keep it small.
local function copy_programs()
  if not R:find(":") then return end         -- only on the Amiga
  for _, c in ipairs({ { "pdftex", "pdflatex" }, { "luahbtex", "lualatex" } }) do
    local src, dst = J(BIN, c[1]), J(BIN, c[2])
    if exists(src) and lfs.attributes(dst, "size") ~= lfs.attributes(src, "size") then
      run("Copy " .. quote(src) .. " TO " .. quote(dst) .. " CLONE QUIET")
    end
  end
end

local function configure(force)
  if opt.noconfig then return end
  say("Configuring ...")
  copy_programs()
  if force or changed.formats then make_languages() end
  if force or changed.maps then make_pdftex_map() end
  say("  file name database")
  mktexlsr(DIST)
  mktexlsr(VAR)
  if opt.formats and (force or changed.formats) then
    say("  formats (this takes a while)")
    if not run("Execute " .. quote(J(R, "MakeFormats")) .. " " .. quote(R)) then
      say("  building the formats failed: run Execute TeX:MakeFormats to see why")
    end
  end
  if opt.fontlist and (force or changed.fonts) then
    local tool = J(DIST, "scripts/luaotfload/luaotfload-tool.lua")
    if exists(tool) then
      say("  font list for LuaLaTeX (a while on a 68030)")
      run(quote(J(BIN, "luahbtex")) .. " --luaonly " .. quote(tool) .. " --update")
    end
  end
end

-- ---------------------------------------------------------------------
-- list, info

local function list(what)
  what = what or "installed"
  local t = {}
  for n, p in pairs(idx) do
    if (what == "installed" and installed[n] ~= nil)
      or (what == "schemes" and p.cat == "Scheme")
      or (what == "collections" and p.cat == "Collection")
      or what == "all" then
      t[#t + 1] = n
    end
  end
  table.sort(t)
  for _, n in ipairs(t) do
    local p = idx[n]
    local mark = installed[n] ~= nil and "i " or "  "
    say(mark, n, p and p.desc and ("  -  " .. p.desc) or "")
  end
end

local function info(list)
  for _, n in ipairs(list) do
    local p = idx[n] or die("no package " .. n)
    local all, size, todo, tsize = closure({ n }), 0, 0, 0
    for _, m in ipairs(all) do
      size = size + idx[m].size
      if installed[m] == nil then todo = todo + 1; tsize = tsize + idx[m].size end
    end
    say(n, " (", p.cat or "?", ")", installed[n] ~= nil and ", installed" or "")
    if p.desc then say("  ", p.desc) end
    say("  with what it needs: ", #all, " packages, ", mb(size), "; not yet installed: ",
        todo, ", ", mb(tsize))
  end
end

-- ---------------------------------------------------------------------

if cmd == "install" then
  if #names == 0 then die("install what?") end
  local ok = install(names)
  if not opt.dry and changed.any then configure(false) end
  if ok == false then os.exit(5) end
elseif cmd == "bundle" then
  if #names == 0 then die("which bundle files?") end
  local ok = bundles(names)
  if not opt.dry and changed.any then configure(false) end
  if not ok then os.exit(5) end
elseif cmd == "remove" then
  remove(names)
  if not opt.dry and changed.any then configure(false) end
elseif cmd == "list" then list(names[1])
elseif cmd == "info" then info(names)
elseif cmd == "config" then configure(true)
else die("unknown command " .. cmd) end
