#!/usr/bin/env luajit

local function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

local function read_points()
  local fh
  if arg and arg[1] then
    fh = assert(io.open(arg[1], "r"))
  else
    fh = io.stdin
  end

  local pts = {}

  for line in fh:lines() do
    line = trim(line)
    if line ~= "" then
      local x, y, z = line:match("^(-?%d+),%s*(-?%d+),%s*(-?%d+)$")
      if not x then
        error("Invalid coordinate line: " .. line)
      end
      pts[#pts + 1] = {
        x = tonumber(x),
        y = tonumber(y),
        z = tonumber(z),
      }
    end
  end

  if fh ~= io.stdin then
    fh:close()
  end

  return pts
end

local function dist2(p, q)
  local dx = p.x - q.x
  local dy = p.y - q.y
  local dz = p.z - q.z
  return dx * dx + dy * dy + dz * dz
end

local function build_edges(pts)
  local n = #pts
  local edges = {}
  local idx = 1

  for i = 1, n do
    for j = i + 1, n do
      edges[idx] = {
        a = i,
        b = j,
        d2 = dist2(pts[i], pts[j]),
      }
      idx = idx + 1
    end
  end

  table.sort(edges, function(e1, e2)
    if e1.d2 ~= e2.d2 then
      return e1.d2 < e2.d2
    end
    if e1.a ~= e2.a then
      return e1.a < e2.a
    end
    return e1.b < e2.b
  end)

  return edges
end

local parent = {}
local sz     = {}

local function uf_init(n)
  for i = 1, n do
    parent[i] = i
    sz[i]     = 1
  end
end

local function uf_find(x)
  local r = x
  while parent[r] ~= r do
    r = parent[r]
  end
  -- path compression
  while parent[x] ~= x do
    local p = parent[x]
    parent[x] = r
    x = p
  end
  return r
end

local function uf_union_roots(ra, rb)
  if ra == rb then
    return
  end
  if sz[ra] < sz[rb] then
    ra, rb = rb, ra
  end
  parent[rb] = ra
  sz[ra] = sz[ra] + sz[rb]
end

local function solve_part1(edges, n, K)
  uf_init(n)

  local ecount = #edges
  local limit = K
  if limit > ecount then
    limit = ecount
  end

  for i = 1, limit do
    local e = edges[i]
    local ra = uf_find(e.a)
    local rb = uf_find(e.b)
    if ra ~= rb then
      uf_union_roots(ra, rb)
    end
  end

  local comps = {}
  for i = 1, n do
    if parent[i] == i then
      comps[#comps + 1] = sz[i]
    end
  end

  table.sort(comps, function(a, b) return a > b end)

  local a = comps[1] or 0
  local b = comps[2] or 0
  local c = comps[3] or 0

  return a * b * c
end

local function solve_part2(edges, n, pts)
  if n <= 1 then
    return 0
  end

  uf_init(n)
  local components = n
  local last_a, last_b = nil, nil

  for i = 1, #edges do
    local e = edges[i]
    local ra = uf_find(e.a)
    local rb = uf_find(e.b)

    if ra ~= rb then
      uf_union_roots(ra, rb)
      components = components - 1
      last_a, last_b = e.a, e.b
      if components == 1 then
        break
      end
    end
  end

  if components ~= 1 or not last_a or not last_b then
    return 0
  end

  local xa = pts[last_a].x
  local xb = pts[last_b].x
  return xa * xb
end

local function main()
  local pts = read_points()
  if #pts == 0 then
    io.stderr:write("No points read.\n")
    os.exit(1)
  end

  local edges = build_edges(pts)
  local K = 1000
  local part1 = solve_part1(edges, #pts, K)
  local part2 = solve_part2(edges, #pts, pts)

  print("Part1: " .. part1)
  print("Part2: " .. part2)
end

main()
