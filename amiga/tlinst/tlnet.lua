-- tlnet.lua: download files of a TeX Live repository over plain HTTP.
--
-- Part of the TeX Live installer for AmigaOS.  Runs in texlua (LuaTeX's
-- Lua with luasocket), on the Amiga and on the build host.  Only plain
-- HTTP: AmigaOS 3 has no TLS in texlua.  Integrity comes from the
-- SHA-512 sums in texlive.tlpdb, checked by the caller.
--
--   local net = require("tlnet")
--   local ok, err = net.fetch(url, file, { proxy = ..., size = n,
--                                          progress = function(got, size) end })

local socket = require("socket")
local http   = require("socket.http")
local ltn12  = require("ltn12")

local M = {}

M.TIMEOUT = 60          -- seconds without data before giving up
M.RETRIES = 3
M.USERAGENT = "TeXLive-Amiga-Installer"

-- a sink that counts bytes and calls progress(got, size) now and then
local function counting(sink, size, progress)
  local got, last = 0, -1
  return function(chunk, err)
    if chunk then
      got = got + #chunk
      local step = math.floor(got / 262144)       -- every 256 KB
      if progress and step ~= last then last = step; progress(got, size) end
    end
    return sink(chunk, err)
  end, function() return got end
end

local function once(url, file, opt)
  local tmp = file .. ".part"
  local f, err = io.open(tmp, "wb")
  if not f then return nil, "cannot write " .. tmp .. ": " .. tostring(err) end
  local sink, count = counting(ltn12.sink.file(f), opt.size, opt.progress)
  http.TIMEOUT = M.TIMEOUT
  http.USERAGENT = M.USERAGENT
  local r, code = http.request{
    url = url, sink = sink, proxy = opt.proxy, redirect = true,
  }
  -- ltn12.sink.file closes the file at the end of the data
  if not r then os.remove(tmp); return nil, tostring(code) end
  if code ~= 200 then os.remove(tmp); return nil, "HTTP " .. tostring(code) end
  if opt.size and count() ~= opt.size then
    os.remove(tmp)
    return nil, string.format("got %d bytes, expected %d", count(), opt.size)
  end
  os.remove(file)
  local ok, e = os.rename(tmp, file)
  if not ok then return nil, e end
  return true
end

-- fetch url into file; retries a few times; returns true or nil, error
function M.fetch(url, file, opt)
  opt = opt or {}
  local err
  for i = 1, M.RETRIES do
    local ok
    ok, err = once(url, file, opt)
    if ok then return true end
    if opt.log then opt.log("download failed (" .. err .. "), try " .. i .. " of " .. M.RETRIES) end
    if err:match("^HTTP 4") then break end     -- not found etc.: no use retrying
    socket.sleep(2 * i)
  end
  return nil, err
end

-- read a whole small file from url into a string
function M.get(url, opt)
  opt = opt or {}
  http.TIMEOUT = M.TIMEOUT
  http.USERAGENT = M.USERAGENT
  local t = {}
  local r, code = http.request{ url = url, sink = ltn12.sink.table(t),
                                proxy = opt.proxy, redirect = true }
  if not r then return nil, tostring(code) end
  if code ~= 200 then return nil, "HTTP " .. tostring(code) end
  return table.concat(t)
end

-- hex form of a binary string (sha2.digest512 gives binary)
function M.hex(s)
  return (s:gsub(".", function(c) return string.format("%02x", c:byte()) end))
end

return M
