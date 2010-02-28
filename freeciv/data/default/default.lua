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

-- When creating new ruleset, you should copy this file only if you
-- need to override default one. Usually you should implement your
-- own scripts in ruleset specific script.lua. This way maintaining
-- ruleset is easier as you don't need to keep your own copy of
-- default.lua updated when ever it changes in Freeciv distribution.

-- Get gold from entering a hut.
function hut_get_gold(unit, gold)
  local owner = unit.owner

  notify.event(owner, unit.tile, E.HUT_GOLD, _("You found %d gold."), gold)
  change_gold(owner, gold)
end

-- Get a tech from entering a hut.
function hut_get_tech(unit)
  local owner = unit.owner
  local tech = give_technology(owner, nil, "hut")

  if tech then
    notify.event(owner, unit.tile, E.HUT_TECH,
                 _("You found %s in ancient scrolls of wisdom."),
                 methods_tech_type_name_translation(tech))
    notify.embassies(owner, unit.tile, E.HUT_TECH,
                     _("The %s have acquired %s from ancient scrolls of\
                      wisdom."),
                     methods_nation_type_plural_translation(owner.nation),
                     methods_tech_type_name_translation(tech))
    return true
  else
    return false
  end
end

-- Get a mercenary unit from entering a hut.
function hut_get_mercenaries(unit)
  local owner = unit.owner
  local type = find.role_unit_type('HutTech', owner)

  if not type then
    type = find.role_unit_type('Hut', nil)
  end

  if type then
    notify.event(owner, unit.tile, E.HUT_MERC,
                 _("A band of friendly mercenaries joins your cause."))
    create_unit(owner, unit.tile, type, 0, unit:get_homecity(), -1)
    return true
  else
    return false
  end
end

-- Get new city from hut, or settlers (nomads) if terrain is poor.
function hut_get_city(unit)
  local owner = unit.owner
  local settlers = find.role_unit_type('Cities', owner)

  if unit:is_on_possible_city_tile() then
    create_city(owner, unit.tile, "")
    notify.event(owner, unit.tile, E.HUT_CITY,
                 _("You found a friendly city."))
  else
    if settlers then
      notify.event(owner, unit.tile, E.HUT_SETTLER,
                   _("Friendly nomads are impressed by you, and join you."))
      create_unit(owner, unit.tile, settlers, 0, unit:get_homecity(), -1)
    end
  end
end

-- Get barbarians from hut, unless close to a city or king enters
-- Unit may die: returns true if unit is alive
function hut_get_barbarians(unit)
  local tile = unit.tile
  local type = unit.utype
  local owner = unit.owner

  if unit.tile:city_exists_within_city_radius(true) 
    or type:has_flag('Gameloss') then
    notify.event(owner, unit.tile, E.HUT_BARB_CITY_NEAR,
                 _("An abandoned village is here."))
    return true
  end
  
  local alive = unleash_barbarians(tile)
  if alive then
    notify.event(owner, tile, E.HUT_BARB,
                  _("You have unleashed a horde of barbarians!"));
  else
    notify.event(owner, tile, E.HUT_BARB_KILLED,
                  _("Your %s has been killed by barbarians!"),
                  type:name_translation());
  end
  return alive
end

-- Randomly choose a hut event
function hut_enter_callback(unit)
  local chance = random(0, 11)
  local alive = true

  if chance == 0 then
    hut_get_gold(unit, 25)
  elseif chance == 1 or chance == 2 or chance == 3 then
    hut_get_gold(unit, 50)
  elseif chance == 4 then
    hut_get_gold(unit, 100)
  elseif chance == 5 or chance == 6 or chance == 7 then
    hut_get_tech(unit)
  elseif chance == 8 or chance == 9 then
    if not hut_get_mercenaries(unit) then
      hut_get_gold(unit, 25)
    end
  elseif chance == 10 then
    alive = hut_get_barbarians(unit)
  elseif chance == 11 then
    hut_get_city(unit)
  end

  -- continue processing if unit is alive
  return (not alive)
end

signal.connect("hut_enter", "hut_enter_callback")
