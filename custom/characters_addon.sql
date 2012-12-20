-- Used for persistent character scales, displays, and factions

CREATE TABLE `characters_addon` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0',
  `scale` float unsigned NOT NULL DEFAULT '1',
  `scale_times_changed` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `scale_unlocked` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `display` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `faction` smallint(5) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`),
  KEY `display` (`display`),
  KEY `faction` (`faction`),
  KEY `scale` (`scale`,`scale_times_changed`,`scale_unlocked`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;