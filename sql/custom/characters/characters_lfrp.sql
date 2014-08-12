--
-- Table structure for table `characters_lfrp`
--

CREATE TABLE IF NOT EXISTS `characters_lfrp` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0',
  `content` text NOT NULL,
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `characters_lfrp`
--
ALTER TABLE `characters_lfrp`
 ADD PRIMARY KEY (`guid`);

--
-- Constraints for dumped tables
--

--
-- Constraints for table `characters_lfrp`
--
ALTER TABLE `characters_lfrp`
ADD CONSTRAINT `characters_lfrp_ibfk_1` FOREIGN KEY (`guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE ON UPDATE CASCADE;