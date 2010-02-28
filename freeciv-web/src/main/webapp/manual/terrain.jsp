<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
</head>
<body>
<h1>Terrain</h1>
<div id="bodyContent">
<h3><span class="editsection"><a id="World" name="World" rel="nofollow"></a></span><span
 class="mw-headline">World</span></h3>
<p>By default, the Freeciv world is made of squares arranged in a
rectangular grid whose north and south (or sometimes east and west)
edges end against the polar ice, but whose other two edges connect,
forming a cylinder that can be circumnavigated. Other topologies are
available as a server
option. </p>
<p>Each square contains some kind of terrain, and together they form
larger features like oceans, continents, and mountain ranges.
</p>
<a id="Terrain" name="Terrain" rel="nofollow"></a>
<h3><span class="editsection"><a id="Terrain" name="Terrain"
 rel="nofollow"></a></span> <span class="mw-headline">Terrain</span></h3>
<p>Terrain serves three roles: the theater upon which your units battle
rival civilizations, the landscape across which your units travel, and
the medium which your cities work to produce resources. We shall
consider each role in turn.
</p>
<a id="Combat" name="Combat" rel="nofollow"></a>
<h4><span class="editsection"><a id="Combat" name="Combat"
 rel="nofollow"></a></span> <span class="mw-headline">Combat</span></h4>
<p>Terrain affects combat very simply: when a land unit is attacked,
its defense strength is multiplied by the defense factor ("bonus") of
the terrain beneath it. See the page on combat for details, and the
catalogue below
for which terrains offer bonuses. (Rivers offer an additional defense
bonus of 50%, i.e. the terrain-specific bonus is multiplied by 1.5.) </p>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><img
 style="border: 0px solid ; width: 143px; height: 72px;"
 class="thumbimage" src="/manual/images/Tx.river.png" alt=""
 align="right">
<div class="thumbcaption">River</div>
</div>
</div>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><img class="thumbimage"
 src="/manual/images/Tx.road.png" alt="" border="0" height="72"
 width="143">
<div class="thumbcaption">Road</div>
</div>
</div>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><img class="thumbimage"
 src="/manual/images/Tx.rail.png" alt="" border="0" height="72"
 width="143">
<div class="thumbcaption">Railroad</div>
</div>
</div>
<a id="Transportation" name="Transportation" rel="nofollow"></a>
<h4><span class="editsection"><a id="Transportation"
 name="Transportation" rel="nofollow"></a></span><span
 class="mw-headline">Transportation</span></h4>
<p><i>Sea and air units</i> always expend one movement point to move
one square sea units because they are confined to the ocean
and adjacent cities, and air units ignore terrain completely. Terrain
really only complicates the movement of land units. </p>
<p><i>Land units</i> - movement "speed":
</p>
<ul>
  <li> Moving across easy terrain costs one point per square; moving
onto rough terrain costs more. The cost for each difficult terrain is
given in the catalogue below. </li>
  <li> The explorer, partisan, and alpine troops travel light enough
that moving one square costs only 1/3 point (except that they can use
railroads like anyone else). </li>
  <li> Other land units move for only 1/3 point per square along:
    <ul>
      <li> <i>rivers</i>, which are natural features that cannot be
altered (except by transforming land to sea and back again), and </li>
      <li> <i>roads</i>, which can be built by workers, settlers, and
engineers. </li>
    </ul>
  </li>
  <li> With the railroad advance, roads can be upgraded to <i>railroads</i>
which cost nothing to move along units can move as far as
they want along a railroad in a single turn! Beware that roads and
railroads can be used by any civilization, so an extensive railroad
system may offer your enemies instant movement across your empire.
Railroads cost three settler-turns regardless of terrain. </li>
</ul>
<p><i>Cities</i> always have roads inside, and railroads, when
their owner has that technology, which will connect to
(rail)roads built adjacent to the city.
</p>
<p>With the bridge building advance,
roads and railroads can be built on river squares to bridge them.
</p>
<a id="Working_Terrain" name="Working_Terrain" rel="nofollow"></a>
<h4><span class="editsection"><a id="Working_Terrain"
 name="Working_Terrain" rel="nofollow"></a></span><span
 class="mw-headline">Working Terrain</span></h4>
<p>Squares within range of a city
may be worked. Cities may be built on any terrain except Ocean or
Glacier.
</p>
<p>When cities work terrain there are three products, described further
in working land:
<b>food points, production points, and trade points</b>. These three
are so important that we specify the output of a square simply by
listing them with slashes in between: for example, "1/2/0" describes a
square that each turn when it is being worked produces one food point,
two production points, and no trade points.
</p>
<p>Every type of terrain has some chance of an additional <i>special
resource</i> that boosts one or two of the products. Special resources
such as gems or minerals occur both on land and along coastlines.
Terrain transformation can make resources inaccessible; for instance,
if an Ocean square with fish is transformed to Swamp, there will no
longer be a bonus from fish for that square. Players familiar with the
commercial games may note that the dispersal of Freeciv specials does
not have any regular pattern.
</p>
<p>The catalogue below lists the output of each terrain, both with and
without special resources.
</p>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><img class="thumbimage"
 src="/manual/images/Tx.irrigation.png" alt="" border="0" height="72"
 width="143">
<div class="thumbcaption">Irrigation</div>
</div>
</div>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><img class="thumbimage"
 src="/manual/images/Tx.farmland.png" alt="" border="0" height="72"
 width="143">
<div class="thumbcaption">Farmland</div>
</div>
</div>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><img class="thumbimage"
 src="/manual/images/Tx.mine.png" alt="" border="0" height="72"
 width="143">
<div class="thumbcaption">Mine</div>
</div>
</div>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><img class="thumbimage"
 src="/manual/images/Tx.fortress.png" alt="" border="0" height="86"
 width="143">
<div class="thumbcaption">Fortress</div>
</div>
</div>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><img class="thumbimage"
 src="/manual/images/Tx.airbase.png" alt="" border="0" height="83"
 width="143">
<div class="thumbcaption">Airbase</div>
</div>
</div>
<a id="Improving_Terrain" name="Improving_Terrain" rel="nofollow"></a>
<h3><span class="editsection"><a id="Improving_Terrain"
 name="Improving_Terrain" rel="nofollow"></a></span> <span
 class="mw-headline">Improving Terrain</span></h3>
<a id="Basics" name="Basics" rel="nofollow"></a>
<h4><span class="editsection"><a id="Basics" name="Basics"
 rel="nofollow"></a></span> <span class="mw-headline">Basics</span></h4>
<p>There are several ways to improve terrain. </p>
<p>As soon as they are created, workers, settlers, and engineers can:
</p>
<ol>
  <li> <i>irrigate</i> land to produce more food or </li>
  <li> build a <i>mine</i> to yield more production points </li>
</ol>
<p>(but not both on the same square). Once built, a mine or irrigation
system may be destroyed by further alteration of the terrain, if the
resulting terrain is unsuitable for the improvement. </p>
<p>To irrigate land, the player must have a water source in one of the
four adjoining squares, whether river or ocean or other irrigated land;
but once irrigated, land remains so even if the water source is
removed.
</p>
<p>Terrain not suitable for irrigation or mining can often be altered
to become more suitable to the player's needs  attempting to
irrigate a forest, for example, creates plains (which can then be
irrigated in the normal way). More radical changes ("transformation")
are mentioned below.
</p>
<p><i>Roads</i> and <i>railroads</i> are improvements that have been
mentioned under "Transportation". They can be built on the same tile as
other improvements (such as irrigation). Note that roads and rivers
enhance trade for some types of terrain, as shown in the catalogue
below, and railroads increase by half the production output of a square
(while also retaining any trade bonus from roads).
</p>
<a id="Later_improvements" name="Later_improvements" rel="nofollow"></a>
<h4><span class="editsection"><a id="Later_improvements"
 name="Later_improvements" rel="nofollow"></a></span><span
 class="mw-headline">Later improvements</span></h4>
<p><i>Fortresses</i> (3 turns; requiring Construction) and <i>airbases</i>
(requiring Radio)
are also terrain improvements.
</p>
<p>In a fortress, units are killed one by one (ignoring the <span
 class="mw-redirect">game.ruleset</span>
option "killstack"), and defence is doubled. Fortresses also extend
national borders, and once Invention has been researched,
units in a fortress have increased vision.
</p>
<p>In an airbase, air units (including helicopters) are also killed one
by one
and allowed to refuel, but also open to attacks by land units.
</p>
<p>Both take three settler-turns to complete, regardless of terrain.
Only workers and engineers can build airbases.
</p>
<p>After Refrigeration has been researched, irrigated tiles can be
improved
again by building <i>farmland</i> (at the same cost as irrigation),
increasing by 50% the food production of the tile if the city working
it has a supermarket.
</p>
<p>Only engineers can directly <b>transform land</b>, with the results
detailed in the catalogue below. For transforming swamp to ocean, one
of the eight adjoining squares must be ocean already; and to allow
transforming ocean into swamp at least three of eight adjoining squares
must be land. The new swamp will have a river if built adjacent to some
river square's single mouth.
</p>
<p>Note that engineers have two movement points per turn to invest in
their activities (moving or static), and thus complete all improvements
in half the number of turns specified in the catalogue. </p>
<p>Two or more units working on the same square under the same orders
combine their labor, speeding completion of their project. </p>
<p>When a unit's orders are interrupted (which can be done just with a
click on the unit), its progress is lost. This will also happen if the
unit's former home city is conquered or destroyed.
</p>
<div class="thumb tright">
<div style="width: 145px;" class="thumbinner"><img class="thumbimage"
 src="/manual/images/Tx.village.png" alt="" border="0" height="72"
 width="143">
<div class="thumbcaption">Village</div>
</div>
</div>
<a id="Villages" name="Villages" rel="nofollow"></a>
<h3><span class="editsection"><a id="Villages" name="Villages"
 rel="nofollow"></a></span><span class="mw-headline">Villages</span></h3>
<p>The user who hosts the game has the option of including up to 500
villages (also called huts), primitive communities spread across the
world at the beginning of the game. Any land unit can enter a village,
making the village disappear and deliver a random response. If the
village proves hostile, it could produce barbarians or the unit
entering may simply be destroyed. If they are friendly, the player
could receive gold, a new technology, a military unit (occasionally a
settler; and sometimes a unit that the player cannot yet create), or
even a new city.
</p>
<p>Later in the game, helicopters may also enter villages, but
overflight by other aircraft will cause the villagers to take fright
and disband.
</p>
<a id="Terrain_Catalog" name="Terrain_Catalog" rel="nofollow"></a>
<h3><span class="editsection"><a id="Terrain_Catalog"
 name="Terrain_Catalog" rel="nofollow"></a></span><span
 class="mw-headline">Terrain Catalog</span></h3>
<table
 style="background: transparent none repeat scroll 0% 50%; -moz-background-clip: -moz-initial; -moz-background-origin: -moz-initial; -moz-background-inline-policy: -moz-initial;"
 border="1" cellpadding="4">
  <tbody>
    <tr bgcolor="#e0f0f0">
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">Terrain </th>
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">F/P/T </th>
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">Special 1 </th>
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">F/P/T </th>
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">Special 2 </th>
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">F/P/T </th>
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">Move cost </th>
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">Defense bonus </th>
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">Irrigation<br>
Mining<br>
Road<br>
result (turns) </th>
      <th
 style="background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);"
 bgcolor="#9bc3d1">Transform to (turns) </th>
    </tr>
    <tr id="Glacier" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Arctic.png"
 alt="Image:arctic.png" border="0" height="48" width="95">
      <p><b>Glacier</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">0/0/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.arctic_ivory.png"
 alt="Image:ts.arctic_ivory.png" border="0" height="48" width="95">
      <p>Ivory </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/1/4 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.arctic_oil.png"
 alt="Image:ts.arctic_oil.png" border="0" height="48" width="95">
      <p>Oil </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">0/3/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">2 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">100% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">impossible<br>
+1 P (10)<br>
+0 T (4) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Tundra (24) </td>
    </tr>
    <tr id="Desert" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Desert.png"
 alt="Image:desert.png" border="0" height="48" width="95">
      <p><b>Desert</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">0/1/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.oasis.png"
 alt="Image:ts.oasis.png" border="0" height="48" width="95">
      <p>Oasis </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">3/1/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.oil.png"
 alt="Image:ts.oil.png" border="0" height="48" width="95">
      <p>Oil </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">0/4/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">1 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">100% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">+1 F (5)<br>
+1 P (5)<br>
+1 T (2) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Plains (24) </td>
    </tr>
    <tr id="Forest" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Forest.png"
 alt="Image:forest.png" border="0" height="48" width="96">
      <p><b>Forest</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/2/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.pheasant.png"
 alt="Image:ts.pheasant.png" border="0" height="48" width="96">
      <p>Pheasant </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">3/2/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.silk.png"
 alt="Image:ts.silk.png" border="0" height="48" width="96">
      <p>Silk </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/2/3 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">2 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">150% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Plains (5)<br>
Swamp (15)<br>
+0 T (4) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Grassland (24) </td>
    </tr>
    <tr id="Grassland" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Grassland.png"
 alt="Image:grassland.png" border="0" height="48" width="95">
      <p><b>Grassland</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">2/0/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.grassland_resources.png"
 alt="Image:ts.grassland_resources.png" border="0" height="48"
 width="95">
      <p>Resources </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">2/1/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.grassland_resources.png"
 alt="Image:ts.grassland_resources.png" border="0" height="48"
 width="95">
      <p>Resources </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">2/1/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">1 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">100% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">+1 F (5)<br>
Forest (10)<br>
+1 T (2) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Hills (24) </td>
    </tr>
    <tr id="Hills" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Hills.png" alt="Image:hills.png"
 border="0" height="48" width="96">
      <p><b>Hills</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/0/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.coal.png"
 alt="Image:ts.coal.png" border="0" height="48" width="96">
      <p>Coal </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/2/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.wine.png"
 alt="Image:ts.wine.png" border="0" height="48" width="96">
      <p>Wine </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/0/4 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">2 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">200% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">+1 F (10)<br>
+3 P (10)<br>
+0 T (4) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Plains (24) </td>
    </tr>
    <tr id="Jungle" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Jungle.png"
 alt="Image:jungle.png" border="0" height="48" width="95">
      <p><b>Jungle</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/0/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.gems.png"
 alt="Image:ts.gems.png" border="0" height="48" width="95">
      <p>Gems </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/0/4 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.fruit.png"
 alt="Image:ts.fruit.png" border="0" height="48" width="95">
      <p>Fruit </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">4/0/1 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">2 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">150% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Grassland (15)<br>
Forest (15)<br>
+0 T (4) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Plains (24) </td>
    </tr>
    <tr id="Mountains" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Mountains.png"
 alt="Image:mountains.png" border="0" height="48" width="96">
      <p><b>Mountains</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">0/1/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.gold.png"
 alt="Image:ts.gold.png" border="0" height="48" width="96">
      <p>Gold </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">0/1/6 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.iron.png"
 alt="Image:ts.iron.png" border="0" height="48" width="96">
      <p>Iron </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">0/4/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">3 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">300% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">impossible<br>
+1 P (10)<br>
+0 T (6) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Hills (24) </td>
    </tr>
    <tr id="Ocean" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ocean.png" alt="Image:ocean.png"
 border="0" height="48" width="95">
      <p><b>Ocean</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/0/2 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.fish.png"
 alt="Image:ts.fish.png" border="0" height="48" width="95">
      <p>Fish </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">3/0/2 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.whales.png"
 alt="Image:ts.whales.png" border="0" height="48" width="95">
      <p>Whales </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">2/1/2 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">1 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">100% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">impossible<br>
impossible<br>
impossible </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Swamp (36) </td>
    </tr>
    <tr id="Plains" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Plains.png"
 alt="Image:plains.png" border="0" height="48" width="95">
      <p><b>Plains</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/1/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.buffalo.png"
 alt="Image:ts.buffalo.png" border="0" height="48" width="95">
      <p>Buffalo </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/3/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.wheat.png"
 alt="Image:ts.wheat.png" border="0" height="48" width="95">
      <p>Wheat </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">3/1/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">1 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">100% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">+1 F (5)<br>
Forest (15)<br>
+1 T (2) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Grassland (24) </td>
    </tr>
    <tr id="Swamp" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Swamp.png" alt="Image:swamp.png"
 border="0" height="48" width="95">
      <p><b>Swamp</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/0/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.peat.png"
 alt="Image:ts.peat.png" border="0" height="48" width="95">
      <p>Peat </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/4/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.spice.png"
 alt="Image:ts.spice.png" border="0" height="48" width="95">
      <p>Spice </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">3/0/4 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">2 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">150% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Grassland (15)<br>
Forest (15)<br>
+0 T (4) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Ocean (36) </td>
    </tr>
    <tr id="Tundra" bgcolor="#e0f0f0">
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Tundra.png"
 alt="Image:tundra.png" border="0" height="48" width="95">
      <p><b>Tundra</b> </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">1/0/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.tundra_game.png"
 alt="Image:ts.tundra_game.png" border="0" height="48" width="95">
      <p>Game </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">3/1/0 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left"><img src="/manual/images/Ts.furs.png"
 alt="Image:ts.furs.png" border="0" height="48" width="95">
      <p>Furs </p>
      </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">2/0/3 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">1 </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="center">100% </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">+1 F (5)<br>
impossible<br>
+0 T (2) </td>
      <td
 style="background-color: rgb(51, 51, 51); color: rgb(255, 255, 255);"
 align="left">Desert (24) </td>
    </tr>
  </tbody>
</table>
<hr>
<p><br>
</p>
<p><br>
</p>
</div>
</body>
</html>
