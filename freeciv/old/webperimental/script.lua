-- Freeciv - Copyright (C) 2007 - The Freeciv Project
--   This program is free software; you can redistribute it and/or modify
--   it under the terms of the GNU General Public License as published by
--   the Free Software Foundation; either version 2, or (at your option)
--   any later version.
--
--   This program is distributed in the hope that it will be useful,
--   but WITHOUT ANY WARRANTY; without even the implied warranty of
--   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--   GNU General Public License for more details.

-- This file is for lua-functionality that is specific to a given
-- ruleset. When freeciv loads a ruleset, it also loads script
-- file called 'default.lua'. The one loaded if your ruleset
-- does not provide an override is default/default.lua.


-- Place Ruins at the location of the destroyed city.
function city_destroyed_callback(city, loser, destroyer)
  city.tile:create_extra("Ruins", NIL)
  -- continue processing
  return false
end

signal.connect("city_destroyed", "city_destroyed_callback")

-- Add random labels to the map.
function place_map_labels()
  local mountains = 0
  local deep_oceans = 0
  local deserts = 0
  local glaciers = 0
  local selected_mountain = 0
  local selected_ocean = 0
  local selected_desert = 0
  local selected_glacier = 0

  -- Count the tiles that has a terrain type that may get a label.
  for place in whole_map_iterate() do
    local terr = place.terrain
    local tname = terr:rule_name()
    if tname == "Mountains" then
      mountains = mountains + 1
    elseif tname == "Deep Ocean" then
      deep_oceans = deep_oceans + 1
    elseif tname == "Desert" then
      deserts = deserts + 1
    elseif tname == "Glacier" then
      glaciers = glaciers + 1
    end
  end

  -- Decide if a label should be included and, in case it should, where.
  if random(1, 100) <= 75 then
    selected_mountain = random(1, mountains)
  end
  if random(1, 100) <= 75 then
    selected_ocean = random(1, deep_oceans)
  end
  if random(1, 100) <= 75 then
    selected_desert = random(1, deserts)
  end
  if random(1, 100) <= 75 then
    selected_glacier = random(1, glaciers)
  end

  -- Place the included labels at the location determined above.
  for place in whole_map_iterate() do
    local terr = place.terrain
    local tname = terr:rule_name()

    if tname == "Mountains" then
      selected_mountain = selected_mountain - 1
      if selected_mountain == 0 then
        place:set_label(_("Highest Peak"))
      end
    elseif tname == "Deep Ocean" then
      selected_ocean = selected_ocean - 1
      if selected_ocean == 0 then
        place:set_label(_("Deep Trench"))
      end
    elseif tname == "Desert" then
      selected_desert = selected_desert - 1
      if selected_desert == 0 then
        place:set_label(_("Scorched Spot"))
      end
    elseif tname == "Glacier" then
      selected_glacier = selected_glacier - 1
      if selected_glacier == 0 then
        place:set_label(_("Frozen Lake"))
      end
    end
  end

  return false
end

signal.connect("map_generated", "place_map_labels")
