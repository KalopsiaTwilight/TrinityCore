DROP TABLE IF EXISTS `morphs`;
CREATE TABLE IF NOT EXISTS `morphs` (
  `guid` bigint(20) unsigned NOT NULL COMMENT 'CHARACTER identifier',
  `name` varchar(255) NOT NULL COMMENT 'Custom given name for morph',
  `id_display` int(10) unsigned NOT NULL DEFAULT '0',
  `id_bnet_gm` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'GM Account Identifier',
  PRIMARY KEY (`guid`,`id_display`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='.f morph table';

-- Data exporting was unselected.
-- Dumping structure for table freedom.private_tele
DROP TABLE IF EXISTS `private_tele`;
CREATE TABLE IF NOT EXISTS `private_tele` (
  `name` varchar(255) NOT NULL DEFAULT '',
  `position_x` float NOT NULL DEFAULT '0',
  `position_y` float NOT NULL DEFAULT '0',
  `position_z` float NOT NULL DEFAULT '0',
  `orientation` float NOT NULL DEFAULT '0',
  `map` int(10) unsigned NOT NULL DEFAULT '0',
  `id_bnet_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'OWNER Account Identifier',
  PRIMARY KEY (`name`,`id_bnet_account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='.f pteleport table';

-- Data exporting was unselected.
-- Dumping structure for table freedom.public_spell
DROP TABLE IF EXISTS `public_spell`;
CREATE TABLE IF NOT EXISTS `public_spell` (
  `spell_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `allow_targeting` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `name` varchar(255) NOT NULL DEFAULT '',
  `id_bnet_gm` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'GM Account Identifier',
  PRIMARY KEY (`spell_id`)
) ENGINE=InnoDB AUTO_INCREMENT=45949 DEFAULT CHARSET=utf8;

-- Data exporting was unselected.
-- Dumping structure for table freedom.public_tele
DROP TABLE IF EXISTS `public_tele`;
CREATE TABLE IF NOT EXISTS `public_tele` (
  `name` varchar(255) NOT NULL DEFAULT '',
  `position_x` float NOT NULL DEFAULT '0',
  `position_y` float NOT NULL DEFAULT '0',
  `position_z` float NOT NULL DEFAULT '0',
  `orientation` float NOT NULL DEFAULT '0',
  `map` int(10) unsigned NOT NULL DEFAULT '0',
  `id_bnet_gm` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'GM Account Identifier',
  PRIMARY KEY (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='.f teleport table';

DROP TABLE IF EXISTS `item_template_extra`;
CREATE TABLE IF NOT EXISTS `item_template_extra` (
  `entry_id` int(10) unsigned NOT NULL,
  `hidden` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `name` VARCHAR(255) NULL DEFAULT '',
  `inventory_type` INT UNSIGNED NULL DEFAULT '0',
  `class` INT UNSIGNED NULL DEFAULT '0',
  `subclass` INT UNSIGNED NULL DEFAULT '0',
  PRIMARY KEY (`entry_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `gameobject_extra`;
CREATE TABLE IF NOT EXISTS `gameobject_extra` (
  `guid` bigint(20) unsigned NOT NULL,
  `scale` float NOT NULL DEFAULT '1',
  `id_creator_bnet` int(10) unsigned NOT NULL DEFAULT '0',
  `id_creator_player` bigint(20) unsigned NOT NULL DEFAULT '0',
  `id_modifier_bnet` int(10) unsigned NOT NULL DEFAULT '0',
  `id_modifier_player` bigint(20) unsigned NOT NULL DEFAULT '0',
  `created` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `modified` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `phaseMask` int(10) unsigned NOT NULL DEFAULT '1',
  `uses_quat` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `roll` FLOAT NOT NULL DEFAULT '0',
  `pitch` FLOAT NOT NULL DEFAULT '0',
  `yaw` FLOAT NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.
-- Dumping structure for table freedom.gameobject_template_extra
DROP TABLE IF EXISTS `gameobject_template_extra`;
CREATE TABLE IF NOT EXISTS `gameobject_template_extra` (
  `id_entry` int(10) unsigned NOT NULL,
  `disabled` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `model_name` varchar(255) NOT NULL DEFAULT '',
  `model_type` varchar(5) NOT NULL DEFAULT 'MDX',
  `is_default` tinyint(4) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `creature_extra`;
CREATE TABLE IF NOT EXISTS `creature_extra` (
  `guid` bigint(20) unsigned NOT NULL,
  `scale` float NOT NULL DEFAULT '-1',
  `id_creator_bnet` int(10) unsigned NOT NULL DEFAULT '0',
  `id_creator_player` bigint(20) unsigned NOT NULL DEFAULT '0',
  `id_modifier_bnet` int(10) unsigned NOT NULL DEFAULT '0',
  `id_modifier_player` bigint(20) unsigned NOT NULL DEFAULT '0',
  `created` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `modified` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `phaseMask` int(10) unsigned NOT NULL DEFAULT '1',
  `displayLock` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `displayId` int(10) unsigned NOT NULL DEFAULT '0',
  `nativeDisplayId` int(10) unsigned NOT NULL DEFAULT '0',
  `genderLock` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `gender` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `swim` TINYINT UNSIGNED NOT NULL DEFAULT '1',
  `gravity` TINYINT UNSIGNED NOT NULL DEFAULT '1',
  `fly` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Dumping structure for table freedom.creature_template_extra
DROP TABLE IF EXISTS `creature_template_extra`;
CREATE TABLE IF NOT EXISTS `creature_template_extra` (
  `id_entry` int(10) unsigned NOT NULL,
  `disabled` tinyint(4) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

ALTER TABLE `item_template_extra`
  ADD INDEX `NameIdx` (`name`);

DROP TABLE IF EXISTS `pet_extra`;
CREATE TABLE IF NOT EXISTS `pet_extra` (
  `id` int(10) unsigned NOT NULL,
  `owner` bigint(20) NOT NULL DEFAULT '0',
  `scale` float NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `character_extra`;
CREATE TABLE IF NOT EXISTS `character_extra` (
  `guid` bigint(20) unsigned NOT NULL,
  `display` bigint(20) NOT NULL DEFAULT '0',
  `scale` float NOT NULL DEFAULT '1',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `updates`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `updates` (
  `name` varchar(200) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'filename with extension of the update.',
  `hash` char(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT '' COMMENT 'sha1 hash of the sql file.',
  `state` enum('RELEASED','ARCHIVED') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if an update is released or archived.',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'timestamp when the query was applied.',
  `speed` int unsigned NOT NULL DEFAULT '0' COMMENT 'time the query takes to apply in ms.',
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='List of all applied updates in this database.';

DROP TABLE IF EXISTS `updates_include`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `updates_include` (
  `path` varchar(200) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'directory to include. $ means relative to the source directory.',
  `state` enum('RELEASED','ARCHIVED') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if the directory contains released or archived updates.',
  PRIMARY KEY (`path`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='List of directories where we want to include sql updates.';

LOCK TABLES `updates_include` WRITE;
INSERT INTO `updates_include` VALUES
('$/sql/updates/freedom','RELEASED');
UNLOCK TABLES;
