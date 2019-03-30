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
-- ruleset is easier as you do not need to keep your own copy of
-- default.lua updated when ever it changes in Freeciv distribution.

-- Get gold from entering a hut.
function _deflua_hut_get_gold(unit, gold)
  local owner = unit.owner

  notify.event(owner, unit.tile, E.HUT_GOLD, PL_("You found %d gold.",
                                                 "You found %d gold.", gold),
               gold)
  owner:change_gold(gold)
end

-- Default if intended hut behavior wasn`t possible.
function _deflua_hut_consolation_prize(unit)
  _deflua_hut_get_gold(unit, 25)
end

-- Get a tech from entering a hut.
function _deflua_hut_get_tech(unit)
  local owner = unit.owner
  local tech = owner:give_tech(nil, -1, false, "hut")

  if tech then
    notify.event(owner, unit.tile, E.HUT_TECH,
                 _("You found %s in ancient scrolls of wisdom."),
                 tech:name_translation())
    notify.research(owner, false, E.TECH_GAIN,
                 -- /* TRANS: One player got tech for the whole team. */
                 _("The %s found %s in ancient scrolls of wisdom for you."),
                 owner.nation:plural_translation(),
                 tech:name_translation())
    notify.research_embassies(owner, E.TECH_EMBASSY,
                 -- /* TRANS: first %s is leader or team name */
                 _("%s has acquired %s from ancient scrolls of wisdom."),
                 owner:research_name_translation(),
                 tech:name_translation())
    return true
  else
    return false
  end
end

-- Get a mercenary unit from entering a hut.
function _deflua_hut_get_mercenaries(unit)
  local owner = unit.owner
  local type = find.role_unit_type('HutTech', owner)

  if not type or not type:can_exist_at_tile(unit.tile) then
    type = find.role_unit_type('Hut', nil)
    if not type or not type:can_exist_at_tile(unit.tile) then
      type = nil
    end
  end

  if type then
    notify.event(owner, unit.tile, E.HUT_MERC,
                 _("A band of friendly mercenaries joins your cause."))
    owner:create_unit(unit.tile, type, 0, unit:get_homecity(), -1)
    return true
  else
    return false
  end
end

-- Get new city from hut, or settlers (nomads) if terrain is poor.
function _deflua_hut_get_city(unit)
  local owner = unit.owner
  local settlers = find.role_unit_type('Cities', owner)

  if unit:is_on_possible_city_tile() then
    owner:create_city(unit.tile, "")
    notify.event(owner, unit.tile, E.HUT_CITY,
                 _("You found a friendly city."))
    return true
  else
    if settlers and settlers:can_exist_at_tile(unit.tile) then
      notify.event(owner, unit.tile, E.HUT_SETTLER,
                   _("Friendly nomads are impressed by you, and join you."))
      owner:create_unit(unit.tile, settlers, 0, unit:get_homecity(), -1)
      return true
    else
      return false
    end
  end
end

-- Get barbarians from hut, unless close to a city, king enters, or
-- barbarians are disabled
-- Unit may die: returns true if unit is alive
function _deflua_hut_get_barbarians(unit)
  local tile = unit.tile
  local type = unit.utype
  local owner = unit.owner

  if server.setting.get("barbarians") == "DISABLED"
    or unit.tile:city_exists_within_max_city_map(true)
    or type:has_flag('Gameloss') then
      notify.event(owner, unit.tile, E.HUT_BARB_CITY_NEAR,
                   _("An abandoned village is here."))
    return true
  end
  
  local alive = tile:unleash_barbarians()
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
function _deflua_hut_enter_callback(unit)
  local chance = random(0, 11)
  local alive = true

  if chance == 0 then
    _deflua_hut_get_gold(unit, 25)
  elseif chance == 1 or chance == 2 or chance == 3 then
    _deflua_hut_get_gold(unit, 50)
  elseif chance == 4 then
    _deflua_hut_get_gold(unit, 100)
  elseif chance == 5 or chance == 6 or chance == 7 then
    _deflua_hut_get_tech(unit)
  elseif chance == 8 or chance == 9 then
    if not _deflua_hut_get_mercenaries(unit) then
      _deflua_hut_consolation_prize(unit)
    end
  elseif chance == 10 then
    alive = _deflua_hut_get_barbarians(unit)
  elseif chance == 11 then
    if not _deflua_hut_get_city(unit) then
      _deflua_hut_consolation_prize(unit)
    end
  end

  -- continue processing if unit is alive
  return (not alive)
end

signal.connect("hut_enter", "_deflua_hut_enter_callback")


--[[
  Make partisans around conquered city

  if requirements to make partisans when a city is conquered is fulfilled
  this routine makes a lot of partisans based on the city`s size.
  To be candidate for partisans the following things must be satisfied:
  1) The loser of the city is the original owner.
  2) The Inspire_Partisans effect must be larger than zero.

  If these conditions are ever satisfied, the ruleset must have a unit
  with the Partisan role.

  In the default ruleset, the requirements for inspiring partisans are:
  a) Guerilla warfare must be known by atleast 1 player
  b) The player must know about Communism and Gunpowder
  c) The player must run either a democracy or a communist society.
]]--

function _deflua_make_partisans_callback(city, loser, winner, reason)
  if reason ~= 'conquest' or city:inspire_partisans(loser) <= 0 then
    return
  end

  local partisans = random(0, 1 + (city.size + 1) / 2) + 1
  if partisans > 8 then
    partisans = 8
  end
  city.tile:place_partisans(loser, partisans, city:map_sq_radius())
  notify.event(loser, city.tile, E.CITY_LOST,
      _("The loss of %s has inspired partisans!"), city.name)
  notify.event(winner, city.tile, E.UNIT_WIN_ATT,
      _("The loss of %s has inspired partisans!"), city.name)
end

signal.connect("city_transferred", "_deflua_make_partisans_callback")


-- Notify player about the fact that disaster had no effect if that is
-- the case
function _deflua_harmless_disaster_message(disaster, city, had_internal_effect)
  if not had_internal_effect then
    notify.event(city.owner, city.tile, E.DISASTER,
        _("We survived the disaster without serious damage."))
  end
end

signal.connect("disaster_occurred", "_deflua_harmless_disaster_message")
