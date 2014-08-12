--
-- Table structure for table `custom_npc_tele_destination`
--

CREATE TABLE IF NOT EXISTS `custom_npc_tele_destination` (
`id` int(6) unsigned NOT NULL,
  `name` char(100) NOT NULL DEFAULT '',
  `pos_X` float NOT NULL DEFAULT '0',
  `pos_Y` float NOT NULL DEFAULT '0',
  `pos_Z` float NOT NULL DEFAULT '0',
  `map` smallint(5) unsigned NOT NULL DEFAULT '0',
  `orientation` float NOT NULL DEFAULT '0',
  `level` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `cost` int(10) unsigned NOT NULL DEFAULT '0',
  `phase` smallint(5) unsigned NOT NULL DEFAULT '1'
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=149 ;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `custom_npc_tele_destination`
--
ALTER TABLE `custom_npc_tele_destination`
 ADD PRIMARY KEY (`id`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `custom_npc_tele_destination`
--
ALTER TABLE `custom_npc_tele_destination`
MODIFY `id` int(6) unsigned NOT NULL AUTO_INCREMENT;
