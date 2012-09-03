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

function city_destroyed_callback(city, loser, destroyer)
  city.tile:create_base("Ruins", NIL)
  -- continue processing
  return false
end

signal.connect("city_destroyed", "city_destroyed_callback")

function place_map_labels()
  local mountains = 0
  local deep_oceans = 0
  local deserts = 0
  local glaciers = 0
  local selected

  for place in whole_map_iterate() do
    local terr = place.terrain
    local tname = terr:rule_name()
    if tname == "Mountains" then
      mountains = mountains + 1
    end
    if tname == "Deep Ocean" then
      deep_oceans = deep_oceans + 1
    end
    if tname == "Desert" then
      deserts = deserts + 1
    end
    if tname == "Glacier" then
      glaciers = glaciers + 1
    end
  end

  if random(1, 100) <= 75 then
    selected = random(1, mountains)

    for place in whole_map_iterate() do
      local terr = place.terrain
      if terr:rule_name() == "Mountains" then
        selected = selected - 1
        if selected == 0 then
          place:set_label("Highest Peak")
        end
      end
    end
  end

  if random(1, 100) <= 75 then
    selected = random(1, deep_oceans)

    for place in whole_map_iterate() do
      local terr = place.terrain
      if terr:rule_name() == "Deep Ocean" then
        selected = selected - 1
        if selected == 0 then
          place:set_label("Deep Trench")
        end
      end
    end
  end

  if random(1, 100) <= 75 then
    selected = random(1, deserts)

    for place in whole_map_iterate() do
      local terr = place.terrain
      if terr:rule_name() == "Desert" then
        selected = selected - 1
        if selected == 0 then
          place:set_label("Scorched spot")
        end
      end
    end
  end

  if random(1, 100) <= 75 then
    selected = random(1, glaciers)

    for place in whole_map_iterate() do
      local terr = place.terrain
      if terr:rule_name() == "Glacier" then
        selected = selected - 1
        if selected == 0 then
          place:set_label("Frozen lake")
        end
      end
    end
  end

  return false
end

signal.connect("map_generated", "place_map_labels")
