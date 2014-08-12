--
-- Table structure for table `gameobject_teleport`
--

CREATE TABLE IF NOT EXISTS `gameobject_teleport` (
  `id` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `name` text,
  `target_map` smallint(5) unsigned NOT NULL DEFAULT '0',
  `target_position_x` float NOT NULL DEFAULT '0',
  `target_position_y` float NOT NULL DEFAULT '0',
  `target_position_z` float NOT NULL DEFAULT '0',
  `target_orientation` float NOT NULL DEFAULT '0',
  `req_level` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `phase` int(10) unsigned NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `gameobject_teleport`
--
ALTER TABLE `gameobject_teleport`
 ADD PRIMARY KEY (`id`);