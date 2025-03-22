REPLACE INTO `commands` (command, syntax, description, gmlevel) VALUES

('customnpc unequip left', 'Syntax: .customnpc unequip left $key [$equipVariation]','Unequips the left hand slot for equipment variation $equipVariation (default 1) of the custom NPC with key $key.', 1),
('customnpc unequip ranged', 'Syntax: .customnpc unequip ranged $key [$equipVariation]','Unequips the ranged slot for equipment variation $equipVariation (default 1) of the custom NPC with key $key.', 1),
('customnpc unequip right', 'Syntax: .customnpc unequip right $key [$equipVariation]','Unequips the right hand slot for equipment variation $equipVariation (default 1) of the custom NPC with key $key.', 1)
;