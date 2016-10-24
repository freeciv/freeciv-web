#! /usr/bin/env python

import wikipedia
import json
from subprocess import call

freeciv_wiki_doc = {};

def fix_tech(tech_name):
  if (tech_name == "Advanced Flight"): tech_name = "Aeronautics";
  if (tech_name == "Rocketry"): tech_name = "Rocket";
  if (tech_name == "Stealth"): tech_name = "Stealth technology";
  if (tech_name == "Tactics"): tech_name = "Military tactics";
  if (tech_name == "The Republic"): tech_name = "Repubic";
  if (tech_name == "The Corporation"): tech_name = "Corporation";
  if (tech_name == "The Wheel"): tech_name = "Wheel";
  if (tech_name == "Archers"): tech_name = "Archery";
  if (tech_name == "Partisan"): tech_name = "Partisan military";
  if (tech_name == "Horsemen"): tech_name = "Equestrianism";
  if (tech_name == "Fighter"): tech_name = "Fighter aircraft";
  if (tech_name == "Carrier"): tech_name = "Aircraft carrier";
  if (tech_name == "Legion"): tech_name = "Roman legion";
  if (tech_name == "Magellan's Expedition"): tech_name = "Ferdinand Magellan";
  if (tech_name == "Nuclear"): tech_name = "Nuclear weapon";
  if (tech_name == "Caravan"): tech_name = "Camel train";
  if (tech_name == "Aqueduct"): tech_name = "Aqueduct water supply";
  if (tech_name == "Barracks II"): tech_name = "Barracks";
  if (tech_name == "Barracks III"): tech_name = "Barracks";
  if (tech_name == "Mfg. Plant"): tech_name = "Factory";
  if (tech_name == "Sewer System"): tech_name = "Sanitary_sewer";
  if (tech_name == "Space Structural"): tech_name = "Spacecraft";
  if (tech_name == "A.Smith's Trading Co."): tech_name = "Adam Smith";
  if (tech_name == "Colossus"): tech_name = "Colossus of Rhodes";
  if (tech_name == "Michelangelo's Chapel"): tech_name = "Sistine Chapel";
  if (tech_name == "Shakespeare's Theater"): tech_name = "William Shakespeare";
  if (tech_name == "Coinage"): tech_name = "Coining (mint)";
  if (tech_name == "SDI Defense"): tech_name = "Strategic Defense Initiative";
  if (tech_name == "Coastal Defense"): tech_name = "Coastal defence and fortification";
  if (tech_name == "Copernicus' Observatory"): tech_name = "Space observatory";
  if (tech_name == "Mech. Inf."): tech_name = "Mechanized infantry";
  if (tech_name == "Sun Tzu's War Academy"): tech_name = "Sun Tzu";
  if (tech_name == "Mobile Warfare"): tech_name = "Mobile_Warfare";
  if (tech_name == "Leonardo's Workshop"): tech_name = "Leonardo da Vinci";
  if (tech_name == "SETI Program"): tech_name = "Search for extraterrestrial intelligence";
  return tech_name;

def validate_image(image_url):
  return ((".png" in image_url.lower() or ".jpg" in image_url.lower()) 
		  and not "Ambox" in image_url and "Writing_systems_worldwide" not in image_url);

def download_wiki_page(tech_name):
  print(tech_name + " -> " + fix_tech(tech_name));
  page = wikipedia.page(fix_tech(tech_name), auto_suggest=True, redirect=True);

  image = None;
  if (len(page.images) > 0 and validate_image(page.images[0])): image = page.images[0];
  if (len(page.images) > 1 and validate_image(page.images[1])): image = page.images[1];
  if (len(page.images) > 2 and validate_image(page.images[2])): image = page.images[2];
  if (len(page.images) > 3 and validate_image(page.images[3])): image = page.images[3];
  if (len(page.images) > 4 and validate_image(page.images[4])): image = page.images[4];
  if image != None:
    image = image.replace("http:", "");  #protocol relative url

  freeciv_wiki_doc[tech_name] = {"title" : page.title, "summary" : page.summary, "image" : image};


# FIXME: extract item names from the other supported rulesets too. An item
# name that don't appear in classic may still appear in another supported
# ruleset.

f = open('../freeciv/freeciv/data/classic/techs.ruleset')
lines = f.readlines()
f.close()

techs = []
for line in lines:
  if line.startswith("name"):
    tech_line = line.split("\"");
    techs.append(tech_line[1]);

f = open('../freeciv/freeciv/data/classic/units.ruleset')
lines = f.readlines()
f.close()

for line in lines:
  if line.startswith("name"):
    tech_line = line.split("\"");
    if ("unitclass" in tech_line[1]): continue;
    result_tech = tech_line[1].replace("?unit:", "");
    techs.append(result_tech);

f = open('../freeciv/freeciv/data/classic/buildings.ruleset')
lines = f.readlines()
f.close()

for line in lines:
  if line.startswith("name"):
    tech_line = line.split("\"");
    techs.append(tech_line[1]);

f = open('../freeciv/freeciv/data/classic/governments.ruleset')
lines = f.readlines()
f.close()

for line in lines:
  if line.startswith("name"):
    tech_line = line.split("\"");
    techs.append(tech_line[1]);

for tech in techs:
  download_wiki_page(tech);

f = open('../freeciv-web/src/main/webapp/javascript/freeciv-wiki-doc.js' ,'w');
f.write("var freeciv_wiki_docs = " + json.dumps(freeciv_wiki_doc) + ";");
f.close();
print("Downloading tech summaries from Wikipedia complete!");
