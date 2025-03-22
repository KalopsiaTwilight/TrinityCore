/**
* PERMISSION GROUP STRUCTURE:
* NOTE: Lesser ID PERM GROUP inherits all permissions from higher ID PERM GROUP.
*       e.g. PERM GROUP 198 (mod) has all permissions of PERM GROUP 199 (player)
* 196 - Role: [GM3] Administrator
* 197 - Role: [GM2] Gamemaster
* 198 - Role: [GM1] Moderator, ST
* 199 - Role: [GM0] Player
*/
START TRANSACTION;
SET @PLAYER             := 195;
SET @MODERATOR          := 194;
SET @GM                 := 193;
SET @ADMIN              := 192;

-- [RECREATION: CUSTOM PERMS]
REPLACE INTO rbac_permissions (`id`, `name`) VALUES
(1380, 'Command: .npc equip left'),
(1381, 'Command: .npc equip ranged'),
(1382, 'Command: .npc equip right'),
(1383, 'Command: .npc unequip left'),
(1384, 'Command: .npc unequip ranged'),
(1385, 'Command: .npc unequip right'),
(1386, 'Command: .npc set equipment')
;

REPLACE INTO rbac_linked_permissions (`id`, `linkedId`) VALUES
(@MODERATOR, 1380),
(@MODERATOR, 1381),
(@MODERATOR, 1382),
(@MODERATOR, 1383),
(@MODERATOR, 1384),
(@MODERATOR, 1385),
(@MODERATOR, 1386)
;
COMMIT;

