--
-- Table structure for table `characters_addon`
--

CREATE TABLE IF NOT EXISTS `characters_addon` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0',
  `scale` float unsigned NOT NULL DEFAULT '1',
  `scale_times_changed` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `scale_unlocked` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `display` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `faction` smallint(5) unsigned NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `characters_addon`
--
ALTER TABLE `characters_addon`
 ADD PRIMARY KEY (`guid`), ADD KEY `display` (`display`), ADD KEY `faction` (`faction`), ADD KEY `scale` (`scale`,`scale_times_changed`,`scale_unlocked`);

--
-- Constraints for dumped tables
--

--
-- Constraints for table `characters_addon`
--
ALTER TABLE `characters_addon`
ADD CONSTRAINT `characters_addon_ibfk_1` FOREIGN KEY (`guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE ON UPDATE CASCADE;