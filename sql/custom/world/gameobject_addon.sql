--
-- Table structure for table `gameobject_addon`
--

CREATE TABLE IF NOT EXISTS `gameobject_addon` (
  `guid` int(10) unsigned NOT NULL,
  `scale` float NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `gameobject_addon`
--
ALTER TABLE `gameobject_addon`
 ADD PRIMARY KEY (`guid`), ADD KEY `guid` (`guid`,`scale`);
