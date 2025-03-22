REPLACE INTO `commands` (command, syntax, description, gmlevel) VALUES
('npc equip left', 'Syntax: .npc equip left ($item|$itemId) [$modAppearanceId]','Equips the linked $item or item with $itemId to the left hand slot to your selected NPC.\nOptionally you can provide a $modAppearanceId to set the modified appearance of the item.', 1),
('npc equip ranged', 'Syntax: .npc equip ranged ($item|$itemId) [$modAppearanceId]','Equips the linked $item or item with $itemId to the ranged slot to your selected NPC.\nOptionally you can provide a $modAppearanceId to set the modified appearance of the item.', 1),
('npc equip right', 'Syntax: .npc equip right ($item|$itemId) [$modAppearanceId]','Equips the linked $item or item with $itemId to the right hand slot to your selected NPC.\nOptionally you can provide a $modAppearanceId to set the modified appearance of the item.', 1),


('npc unequip left', 'Syntax: .npc unequip left [$equipVariation]','Unequips the left hand slot for equipment variation $equipVariation (default 1) for your selected NPC.', 1),
('npc unequip ranged', 'Syntax: .npc unequip ranged [$equipVariation]','Unequips the ranged slot for equipment variation $equipVariation (default 1) for your selected NPC.', 1),
('npc unequip right', 'Syntax: .npc unequip right [$equipVariation]','Unequips the right hand slot for equipment variation $equipVariation (default 1) for your selected NPC.', 1),

('npc set equipment', 'Syntax: .npc set equipment [$equipVariation]','Sets your targetted NPC to equipment variation $equipVariation.', 1)
;