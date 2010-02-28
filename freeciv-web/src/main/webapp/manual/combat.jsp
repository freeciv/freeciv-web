<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
</head>
<body>
<h1>Combat</h1>
<div id="bodyContent">
<p>A unit cannot enter a square occupied by an enemy unit, and when
directed to do so will <b>attack</b> instead, locking the two units in
combat until one is destroyed. An attack usually costs the aggressor
one movement point, but results in no actual motion&nbsp; the
surviving unit remains where it was when the combat started. Bombers
spend all of their remaining movement points when they attack, which
gives fighters a chance to intercept them.
</p>
<p>Some restrictions upon warfare are rather obvious&nbsp; units
must have a nonzero attack strength to attack, while defenders with
zero defense strength lose immediately. There are also limits upon
which units can attack which others. Land units can only attack other
land units. Ships can attack not only other ships, but any land units
adjacent to them (submarines are an exception and cannot attack land
units). Helicopters can attack land and sea units and can be attacked
by any kind of unit (land, sea, or air), at any time. Bombers can
attack anything on land or sea, and though their targets will defend
themselves from attack, they cannot attack the bomber in return. Only
fighters and missiles can attack every kind of unit.
</p>
<p>Note that aircraft within cities and air bases are on the ground,
and thus vulnerable to land attack. Ships in port are similarly
vulnerable. Note also the special ability of marines to attack targets
from aboard ship; other land units must disembark before engaging enemy
units.
</p>
<p>There are two other actions related to combat. A unit ordered to <b>sentry</b>
remains in place indefinitely and no longer asks for orders each turn.
Sentry units can not only be reactivated manually (by selecting them),
but activate automatically should an enemy unit come into view. Land
units can additionally be ordered to <b>fortify</b>, which means they
spend one movement point preparing to be attacked; once fortified they
enjoy the same advantage as land units within an unwalled city. A unit
whose movement points are exhausted cannot fortify&nbsp; it must
have one movement point left at the end of a turn to begin the next
turn fortified.
</p>
<a id="Combat_Mechanics" name="Combat_Mechanics" rel="nofollow"></a>
<h2><span class="editsection"
 style="position: relative; top: -3px; margin-bottom: -2px;"><span
 class="wikia_button"><span></span></span></span><span
 class="mw-headline">Combat Mechanics</span></h2>
<p>Each unit begins combat with one or more <i>hit points</i>, which
are the amount of damage it can sustain. (See the units page for how
many hit points each unit starts with, and for the other combat
statistics discussed on this page.) Combat consists of successive
rounds of violence between the units, which cannot be interrupted and
cease only when one unit is reduced to zero hit points and dies. In
each round one unit succeeds in wounding the other; the damage a unit
inflicts with each blow is called its <i>firepower</i>.
</p>
<p>Which unit inflicts damage on any given round of combat is random.
The attacker has a chance proportional to his <i>attack strength</i>,
while the defender's chance is proportional to his <i>defense strength</i>.
For example, archers (attack stength 3) attacking a phalanx (defense
strength 2) will have a 3/5ths chance of inflicting damage each round,
with the phalanx having the remaining 2/5ths chance. But there are many
factors which affect a unit's strengths, which are all summarized in
the table below. Notice that many bonuses are possible for defenders,
but few for attackers, aside from veteran status; an attacking unit can
mostly expect circumstance to work against it.
</p>
<p>The normal adjustments to combat strength are as follows:
</p>
<table style="margin: auto;">
  <tbody>
    <tr>
      <td colspan="2" align="center" bgcolor="#000000"><b>General
Combat Strength Modifications</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Unit is a veteran at 1st level (veteran) </td>
      <td align="right"><b>&Atilde;&#8212;1.5</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Unit is a veteran at 2nd level (hardened) </td>
      <td align="right"><b>&Atilde;&#8212;1.75</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Unit is a veteran at 3rd level (elite) </td>
      <td align="right"><b>&Atilde;&#8212;2</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td colspan="2" align="center" bgcolor="#000000"><b>Defender
strength adjustments</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Terrain offers defense multiplier m </td>
      <td align="right"><b>&Atilde;&#8212;m</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Terrain includes a river </td>
      <td align="right"><b>&Atilde;&#8212;1.5</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>In city with SAM battery against aircraft (except helicopter)
      </td>
      <td align="right"><b>&Atilde;&#8212;2</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>In city with SDI defense against missile </td>
      <td align="right"><b>&Atilde;&#8212;2</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>In city with coastal defense against ship </td>
      <td align="right"><b>&Atilde;&#8212;2</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>In city with city walls against land unit or helicopter
(except howitzer) </td>
      <td align="right"><b>&Atilde;&#8212;3</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Land unit in fortress </td>
      <td align="right"><b>&Atilde;&#8212;2</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Land unit fortified or in city </td>
      <td align="right"><b>&Atilde;&#8212;1.5</b> </td>
    </tr>
  </tbody>
</table>
<p>These factors are combined, thus a city built on a hill with a river
that has city walls is very hard to conquer.
</p>
<p>There are also combinations of units and circumstances that result
in very specific adjustments to combat:
</p>
<table style="margin: auto;">
  <tbody>
    <tr bgcolor="#000000">
      <td colspan="2" align="center" bgcolor="#000000"><b>Specific
Combat Modifications</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Pikemen attacked by horsemen, chariot, knights, or dragoons </td>
      <td align="right">Defense strength <b>&Atilde;&#8212;2</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>AEGIS Cruiser attacked by an aircraft, missile or helicopter </td>
      <td align="right">Defense strength <b>&Atilde;&#8212;5</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Fighter attacks a helicopter </td>
      <td align="right">Helicopter firepower reduced to <b>1</b><br>
      <p>Helicopter defense strength <b>&Atilde;&#8212;1/2</b> </p>
      </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Ship attacked while inside city </td>
      <td align="right">Attacker firepower <b>&Atilde;&#8212;2</b><br>
Ship firepower reduced to <b>1</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Ship attacks land unit </td>
      <td align="right">Both have firepower reduced to <b>1</b> </td>
    </tr>
  </tbody>
</table>
<a id="Aftermath" name="Aftermath" rel="nofollow"></a>
<h2><span class="editsection"
 style="position: relative; top: -3px; margin-bottom: -2px;"><span
 class="wikia_button"><span></span></span></span><span
 class="mw-headline">Aftermath</span></h2>
<p>Green units have a 50% percent chance of becoming veterans each time
they survive combat (33% for hardened and 20% for elite), which gives
them greater strength in all future engagements.
</p>
<p>Units remain damaged after losing hit points in combat, and will
enter subsequent engagements with this disadvantage. Damaged ground
units also begin each turn with fewer movement points than normal, in
proportion to what fraction of their total hit points remain. To regain
hit points they must spend turns neither moving nor attacking. Resting
in the open restores one hit point; spending a turn fortified restores
two; in a fortress they regain one-quarter of their original hit
points; and in a city they regain a full third of their original
points. The United Nations wonder restores another two points per turn
to all of your units.
</p>
<table style="margin: auto;">
  <tbody>
    <tr>
      <td colspan="2" align="center" bgcolor="#000000"><b>Healing
Factor Modifications</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Resting in the open </td>
      <td align="right"><b>+1 hp</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Fortified </td>
      <td align="right"><b>+2 hp</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>In a city </td>
      <td align="right"><b>+33%</b> </td>
    </tr>
    <tr bgcolor="#000000">
      <td>In a fortress </td>
      <td align="right">Completely restored </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Land unit in city with Barracks </td>
      <td align="right">Completely restored </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Sea unit in city with Port Facility </td>
      <td align="right">Completely restored </td>
    </tr>
    <tr bgcolor="#000000">
      <td>Air unit in city with Airport </td>
      <td align="right">Completely restored </td>
    </tr>
  </tbody>
</table>
<p>If you sentry a damaged unit, it will become active again and demand
further orders when its hit points are fully restored.
</p>
<p>You may enhance the effects of a city upon your units with several
different buildings:
</p>
<table id="bldgs-Units_veteran_status-1" style="margin: auto;">
  <tbody>
    <tr>
      <td>
      <p><br>
      </p>
      <table cellpadding="4" width="100%">
        <tbody>
          <tr bgcolor="#000000">
            <td width="50"><span class="image"><img
 src="http://images3.wikia.nocookie.net/freeciv/images/2/2e/B.barracks.png"
 alt="image:b.barracks.png" border="0" height="64" width="64"></span> </td>
            <td width="200"><b>Barracks </b>cost:<b>30</b> upkeep:<b>1</b>
requires:None </td>
          </tr>
        </tbody>
      </table>
      </td>
      <td>
      <p><br>
      </p>
      <table cellpadding="4" width="100%">
        <tbody>
          <tr bgcolor="#000000">
            <td width="50"><span class="image"><img
 src="http://images3.wikia.nocookie.net/freeciv/images/9/9a/B.barracks2.png"
 alt="image:b.barracks2.png" border="0" height="64" width="64"></span> </td>
            <td width="200"><b>Barracks II </b>cost:<b>30</b> upkeep:<b>1</b>
requires:Gunpowder </td>
          </tr>
        </tbody>
      </table>
      </td>
    </tr>
    <tr>
      <td>
      <p><br>
      </p>
      <table cellpadding="4" width="100%">
        <tbody>
          <tr bgcolor="#000000">
            <td width="50"><span class="image"><img
 src="http://images1.wikia.nocookie.net/freeciv/images/f/fd/B.barracks3.png"
 alt="image:b.barracks3.png" border="0" height="64" width="64"></span> </td>
            <td width="200"><b>Barracks III </b>cost:<b>30</b> upkeep:<b>1</b>
requires:Mobile Warfare </td>
          </tr>
        </tbody>
      </table>
      </td>
    </tr>
  </tbody>
</table>
<table id="bldgs-Units_veteran_status-2" style="margin: auto;">
  <tbody>
    <tr>
      <td>
      <p><br>
      </p>
      <table cellpadding="4" width="100%">
        <tbody>
          <tr bgcolor="#000000">
            <td width="50"><span class="image"><img
 src="http://images2.wikia.nocookie.net/freeciv/images/c/c5/B.port_facility.png"
 alt="image:b.port_facility.png" border="0" height="64" width="64"></span>
            </td>
            <td width="200"><b>Port Facility </b>cost:<b>60</b> upkeep:<b>3</b>
requires:Amphibious Warfare </td>
          </tr>
        </tbody>
      </table>
      </td>
      <td>
      <p><br>
      </p>
      <table cellpadding="4" width="100%">
        <tbody>
          <tr bgcolor="#000000">
            <td width="50"><span class="image"><img
 src="http://images3.wikia.nocookie.net/freeciv/images/4/4a/B.airport.png"
 alt="image:b.airport.png" border="0" height="64" width="64"></span> </td>
            <td width="200"><b>Airport </b>cost:<b>120</b> upkeep:<b>3</b>
requires:Radio </td>
          </tr>
        </tbody>
      </table>
      </td>
    </tr>
  </tbody>
</table>
<a id="Cities_and_Forts" name="Cities_and_Forts" rel="nofollow"></a>
<h2><span class="editsection"
 style="position: relative; top: -3px; margin-bottom: -2px;"><span
 class="wikia_button"><span></span></span></span><span
 class="mw-headline">Cities and Forts</span></h2>
<p>When several units on the same square are attacked, the unit most
capable of defense protects the entire square. If the defenders are
within a city or fortress, the loss of that defender leaves the other
units intact; but outside of such fortification, loss of the defender
results in the loss of every unit in the square. Less surprising is
that when a ship carrying other units is involved in combat (as either
attacker or defender), only the ship participates in the engagement and
its occupants are lost if the ship goes down.
</p>
<p>Defeating a unit inside a city without walls kills one citizen if
the attacker is a land unit. Once the last defender has fallen you may
enter the city and claim it as your own with either a land unit or
helicopter; ships and aircraft can assault cities but not take them.
Upon the capture of a city from another civilization, each building has
a one-fifth chance of being destroyed, and the victor may discover a
technology held by the vanquished.
</p>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><span class="image"><img
 class="thumbimage"
 src="http://images1.wikia.nocookie.net/freeciv/images/f/f7/Tx.fortress.png"
 alt="" border="0" height="86" width="143"></span>
<div class="thumbcaption">fortress</div>
</div>
</div>
<p>Building a fortress requires the construction advance. To begin
construction, move settlers, workers, or engineers to the location at
which you desire a fort and give them the <b>build fortress</b> order.
The work will require only three settler-turns. A fortress can stand
anywhere outside of a city.
</p>
<p>There are several buildings which enhance the strength of units
which are attacked while inside the city:
</p>
<table id="bldgs-City_defense-1" style="margin: auto;">
  <tbody>
    <tr>
      <td>
      <table cellpadding="4" width="100%">
        <tbody>
          <tr bgcolor="#000000">
            <td width="50"><span class="image"><img
 src="http://images4.wikia.nocookie.net/freeciv/images/5/57/B.city_walls.png"
 alt="image:b.city_walls.png" border="0" height="64" width="64"></span>
            </td>
            <td width="200"><b>City Walls</b>
            <p>cost:<b>60</b> upkeep:<b>0</b> </p>
            <p>requires:Masonry </p>
            </td>
          </tr>
        </tbody>
      </table>
      </td>
      <td>
      <table cellpadding="4" width="100%">
        <tbody>
          <tr bgcolor="#000000">
            <td width="50"><span class="image"><img
 src="http://images2.wikia.nocookie.net/freeciv/images/0/02/B.coastal_defense.png"
 alt="image:b.coastal_defense.png" border="0" height="64" width="64"></span>
            </td>
            <td width="200"><b>Coastal Defense</b>
            <p>cost:<b>60</b> upkeep:<b>1</b> </p>
            <p>requires:Metallurgy </p>
            </td>
          </tr>
        </tbody>
      </table>
      </td>
    </tr>
    <tr>
      <td>
      <table cellpadding="4" width="100%">
        <tbody>
          <tr bgcolor="#000000">
            <td width="50"><span class="image"><img
 src="http://images4.wikia.nocookie.net/freeciv/images/c/c4/B.sdi_defense.png"
 alt="image:b.sdi_defense.png" border="0" height="64" width="64"></span>
            </td>
            <td width="200"><b>SDI Defense</b>
            <p>cost:<b>140</b> upkeep:<b>4</b> </p>
            <p>requires:Laser </p>
            </td>
          </tr>
        </tbody>
      </table>
      </td>
      <td>
      <table cellpadding="4" width="100%">
        <tbody>
          <tr bgcolor="#000000">
            <td width="50"><span class="image"><img
 src="http://images2.wikia.nocookie.net/freeciv/images/8/86/B.sam_battery.png"
 alt="image:b.sam_battery.png" border="0" height="64" width="64"></span>
            </td>
            <td width="200"><b>SAM Battery</b>
            <p>cost:<b>70</b> upkeep:<b>2</b> </p>
            <p>requires:Rocketry </p>
            </td>
          </tr>
        </tbody>
      </table>
      </td>
    </tr>
  </tbody>
</table>
<a id="Nuclear_Combat" name="Nuclear_Combat" rel="nofollow"></a>
<h2><span class="editsection"
 style="position: relative; top: -3px; margin-bottom: -2px;"><span
 class="wikia_button"><span></span></span></span><span
 class="mw-headline">Nuclear Combat</span></h2>
<p>Nuclear missiles do not engage in combat like other units&nbsp;
they either strike within range of an SDI Defense and are harmlessly
destroyed, or detonate and blast the entire 3&Atilde;&#8212;3 area centered
on the unit or city they attack. Within the blast area all units are
destroyed, cities lose half their population, and each land square has
a one-half chance of becoming polluted with fallout.
</p>
<p>Just as excessive pollution across the world can trigger <span
 class="mw-redirect">global warming</span>, fallout raises the chances
of <span class="mw-redirect">nuclear winter</span> with the opposite
effect&nbsp; rather than coastlines becoming jungles and swamp,
terrain begins changing into desert and tundra. Settlers, workers, and
engineers must be given the <b>clean fallout</b> command to dispose of
nuclear waste, which costs three settler-turns per square.
</p>
</div>
</body>
</html>
