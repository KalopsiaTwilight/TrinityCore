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
(2000, 'Command: .faddon loginmsg add'),
(2001, 'Command: .faddon loginmsg del'),
(2002, 'Command: .faddon get permlvl')
;

REPLACE INTO rbac_linked_permissions (`id`, `linkedId`) VALUES
(@PLAYER, 2000),
(@PLAYER, 2001),
(@PLAYER, 2002)
;
COMMIT;

