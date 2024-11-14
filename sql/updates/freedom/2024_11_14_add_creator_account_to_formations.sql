ALTER TABLE `formations`
ADD COLUMN `creator_player_id` int(10) unsigned NOT NULL DEFAULT 0;

REPLACE INTO `commands` (command, syntax, description, gmlevel) VALUES
('formation list', 'Syntax: .formation list [-all] [$search]','Lists formations you created. Use -all to see all formations. Optionally filter list by searching for values starting with $search.',1)
;