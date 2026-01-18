DELETE FROM `terrain_swap_defaults` WHERE `TerrainSwapMap` = 1945;
INSERT INTO `terrain_swap_defaults` (`MapId`, `TerrainSwapMap`, `Comment`) VALUES
(0, 1945 , 'Arathi Highlands 2');

DELETE FROM `terrain_worldmap` WHERE `TerrainSwapMap` = 1945;
INSERT INTO `terrain_worldmap` (`TerrainSwapMap`, `UiMapPhaseId`, `Comment`) VALUES
(1945 , 11292, 'Arathi Highlands 2');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 25 AND `SourceEntry` = 1945;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(25, 0, 1945, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 'Arathi Highlands 2');
