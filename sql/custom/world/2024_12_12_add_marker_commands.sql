
REPLACE INTO `command` (name, help) VALUES
('marker add gob', 'Syntax: .marker add gob $entryId\nSpawns a gameobject with entry $entryId at the position of your marker.'),
('marker add npc', 'Syntax: .marker add npc $entryId\nSpawns an NPC with entry $entryId at the position of your marker.'),

('marker goto', 'Syntax: .marker goto\nTells your currently targetted NPC to go the location where you placed your marker with the marker spell.'),
('marker goto formation', 'Syntax: .marker goto formation $formationKey\nTells formation with $formationKey to go to the location where you placed your marker with the marker spell.'),

('marker turnto', 'Syntax: .marker turnto\nTells your currently targetted NPC to turn to the location where you placed your marker with the marker spell.'),
('marker turnto formation', 'Syntax: .marker turnto formation $formationKey\nTells formation with $formationKey to turn to the location where you placed your marker with the marker spell.'),

('marker walkto', 'Syntax: .marker walkto\nTells your currently targetted NPC to walk to the location where you placed your marker with the marker spell.'),
('marker walkto formation', 'Syntax: .marker walkto formation $formationKey\nTells formation with $formationKey to walk to the location where you placed your marker with the marker spell.')

;