local pal = {}
local shades = {}
--Palette script parsing
function char2num (c)
	local x = string.byte (c)
	if x >= string.byte ("A") and x <= string.byte ("F") then
		return x - string.byte ("A") + 10
	end
	if x >= string.byte ("0") and x <= string.byte ("9") then
		return x - string.byte ("0")
	end
	return 0
end
function parsetriplet (triplet)
	triplet = string.upper (triplet)
	if #triplet == 3 then
		local r = 16*char2num (triplet:sub (1, 1))
		local g = 16*char2num (triplet:sub (2, 2))
		local b = 16*char2num (triplet:sub (3, 3))
		return r, g, b
	end
	--Maximal munch the nibble pairs
	local rgb = {0, 0, 0}
	local len = #triplet
	if len%6 == 0 then len = 6
	elseif len%2 ~= 0 then len = len - 1 end
	--Parse nibbles
	local j = 1
	for i = 1, len, 2 do
		rgb[j] = 16*char2num (triplet:sub (i, i))
				+ char2num (triplet:sub (i+1, i+1))
		j = j + 1
	end
	--Handle any remaining nibble
	if #triplet ~= len then
		rgb[j] = 16*char2num (string.byte (triplet, #triplet))
	end
	return rgb[1], rgb[2], rgb[3]
end
function parse (text)
	--Get the black shade if it's there, else assume 000
	local dark = {0, 0, 0}
	local black = text:match ("black (%w+)")
	if black then
		--remove it from the text so we can match other entries
		text = text:gsub ("black %w+", "")
		dark = {parsetriplet (black)}
	end
	--Parse out all the entries
	for n, t in text:gmatch ("(%d+)%s-(%w+)") do
		local num = tonumber (n)
		local c = {parsetriplet (t)}
		--Generate gradient from dark to light
		for i = 1, num do
			if #pal == 255 then
				--Takes into account the dark colour added below
				error ("Palette exceeds 256 entries")
			end
			local k = i/num
			local osk = 1 - k
			local r = math.floor (k*c[1] + osk*dark[1])
			local g = math.floor (k*c[2] + osk*dark[2])
			local b = math.floor (k*c[3] + osk*dark[3])
			table.insert (pal, {r, g, b})
		end
	end
	--Add dark colour to palette
	table.insert (pal, dark)
end
--Shade table generation
function shadegen (grades)
	--First shade is identity
	for i = 1, #pal do
		table.insert (shades, i-1)
	end
	--Calculate each grade of the shade table
	--the final grade is always all black
	for i = 1, grades-1 do
		local k = 1 - i/(grades-1)
		for _, c in ipairs (pal) do
			--Darken the base colour
			local r = k*c[1] + 0.5
			local g = k*c[2] + 0.5
			local b = k*c[3] + 0.5
			--Find the best match in the palette to which we remap
			local best = 1e6
			local id = 0
			for i, pc in ipairs (pal) do
				local R = r - pc[1]
				local G = g - pc[2]
				local B = b - pc[3]
				local delta = R*R + G*G + B*B
				if delta <= best then
					best = delta
					id = i;
					--Early out on exact matches
					if delta < 1e-4 then
						break
					end
				end
			end
			--Add remapped colour to table
			table.insert (shades, id - 1)
		end
	end
end
--Writes everything to disk
function write (file)
	local f = io.open (file, "wb")
	--Write palette
	for _, t in ipairs (pal) do
		f:write (string.char (t[1], t[2], t[3]))
	end	
	--Write shade table
	for _, t in ipairs (shades) do
		f:write (string.char (t))
	end
	f:close ()
end

--Globals
nshades = 32
input = ""
output = "pal.dat"

--Command line parsing
::parseargs::
for i, v in pairs (arg) do
	local pref, cmd = v:sub (1, 1), v:sub (2)
	if pref == "-" then
		local l, r = cmd:match ("(%w+)=(%g+)")
		if l and r then
			_G[l] = r
		end
		table.remove (arg, i)
		goto parseargs
	end
end
--Strip lua and script name from args
arg[-1] = nil
arg[0] = nil
--Ensure header were supplied, ostensibly
if #arg <= 0 then
	print ("lua palgen.lua #shades script")
	return
end
--Load and parse script
local scrf = io.open (arg[2])
local script = scrf:read ("*a")
scrf:close ()

parse (script)
for _, t in ipairs (pal) do
	print (t[1], t[2], t[3])
end
--Generate shade table
shadegen (tonumber (arg[1]))
local s = ""
for _, t in ipairs (shades) do
	s = s .. tostring (t)
	if _%#pal == 0 then
		print (s)
		s = ""
		print ("------")
	elseif #s >= 60 then
		print (s)
		s = ""
	else
		s = s .. " "
	end
end
print (s)
--Shit it all out to disk
write (output)
