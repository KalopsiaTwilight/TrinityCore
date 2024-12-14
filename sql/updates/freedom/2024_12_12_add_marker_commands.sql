REPLACE INTO `commands` (command, syntax, description, gmlevel) VALUES
('marker add gob', 'Syntax: .marker add gob $entryId','Spawns a gameobject with entry $entryId at the position of your marker.',1),
('marker add npc', 'Syntax: .marker add npc $entryId','Spawns an NPC with entry $entryId at the position of your marker.',1),

('marker goto', 'Syntax: .marker goto','Tells your currently targetted NPC to go the location where you placed your marker with the marker spell.', 1),
('marker goto formation', 'Syntax: .marker goto formation $formationKey','Tells formation with $formationKey to go to the location where you placed your marker with the marker spell.', 1),

('marker turnto', 'Syntax: .marker turnto','Tells your currently targetted NPC to turn to the location where you placed your marker with the marker spell.', 1),
('marker turnto formation', 'Syntax: .marker turnto formation $formationKey','Tells formation with $formationKey to turn to the location where you placed your marker with the marker spell.', 1),

('marker walkto', 'Syntax: .marker walkto','Tells your currently targetted NPC to walk to the location where you placed your marker with the marker spell.', 1),
('marker walkto formation', 'Syntax: .marker walkto formation $formationKey','Tells formation with $formationKey to walk to the location where you placed your marker with the marker spell.', 1)

;