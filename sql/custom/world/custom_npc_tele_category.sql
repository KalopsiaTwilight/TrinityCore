CREATE TABLE IF NOT EXISTS `custom_npc_tele_category` (
  `id` int(6) unsigned NOT NULL DEFAULT '0',
  `name` varchar(255) NOT NULL DEFAULT '',
  `flag` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `data0` bigint(20) unsigned NOT NULL DEFAULT '0',
  `data1` int(6) unsigned NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `custom_npc_tele_category`
--
ALTER TABLE `custom_npc_tele_category`
 ADD PRIMARY KEY (`id`);