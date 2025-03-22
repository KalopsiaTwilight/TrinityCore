
REPLACE INTO `command` (name, help) VALUES
('npc equip left', 'Syntax: .npc equip left ($item|$itemId) [$modAppearanceId]\nEquips the linked $item or item with $itemId to the left hand slot to your selected NPC.\nOptionally you can provide a $modAppearanceId to set the modified appearance of the item.'),
('npc equip ranged', 'Syntax: .npc equip ranged ($item|$itemId) [$modAppearanceId]\nEquips the linked $item or item with $itemId to the ranged slot to your selected NPC.\nOptionally you can provide a $modAppearanceId to set the modified appearance of the item.'),
('npc equip right', 'Syntax: .npc equip right ($item|$itemId) [$modAppearanceId]\nEquips the linked $item or item with $itemId to the right hand slot to your selected NPC.\nOptionally you can provide a $modAppearanceId to set the modified appearance of the item.'),


('npc unequip left', 'Syntax: .npc unequip left [$equipVariation]\nUnequips the left hand slot for equipment variation $equipVariation (default 1) for your selected NPC.'),
('npc unequip ranged', 'Syntax: .npc unequip ranged [$equipVariation]\nUnequips the ranged slot for equipment variation $equipVariation (default 1) for your selected NPC.'),
('npc unequip right', 'Syntax: .npc unequip right [$equipVariation]\nUnequips the right hand slot for equipment variation $equipVariation (default 1) for your selected NPC.'),

('npc set equipment', 'Syntax: .npc set equipment [$equipVariation]\nSets your targetted NPC to equipment variation $equipVariation.')

;