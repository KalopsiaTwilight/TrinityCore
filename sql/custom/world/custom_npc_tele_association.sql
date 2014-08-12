CREATE TABLE IF NOT EXISTS `custom_npc_tele_association` (
  `cat_id` int(6) unsigned NOT NULL DEFAULT '0',
  `dest_id` int(6) unsigned NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `custom_npc_tele_association`
--
ALTER TABLE `custom_npc_tele_association`
 ADD PRIMARY KEY (`cat_id`,`dest_id`);
