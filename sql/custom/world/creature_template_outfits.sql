CREATE TABLE IF NOT EXISTS `creature_template_outfits` (
  `entry` int(10) unsigned NOT NULL,
  `race` tinyint(3) unsigned NOT NULL DEFAULT '1',
  `gender` tinyint(3) unsigned NOT NULL DEFAULT '0' COMMENT '0 for male, 1 for female',
  `skin` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `face` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `hair` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `haircolor` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `facialhair` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `head` int(10) NOT NULL DEFAULT '0',
  `shoulders` int(10) NOT NULL DEFAULT '0',
  `body` int(10) NOT NULL DEFAULT '0',
  `chest` int(10) NOT NULL DEFAULT '0',
  `waist` int(10) NOT NULL DEFAULT '0',
  `legs` int(10) NOT NULL DEFAULT '0',
  `feet` int(10) NOT NULL DEFAULT '0',
  `wrists` int(10) NOT NULL DEFAULT '0',
  `hands` int(10) NOT NULL DEFAULT '0',
  `back` int(10) NOT NULL DEFAULT '0',
  `tabard` int(10) NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Use positive values for item entries and negative to use item displayid for head, shoulders etc.';

ALTER TABLE `creature_template_outfits`
 ADD PRIMARY KEY (`entry`);


ALTER TABLE `creature_template`
  CHANGE COLUMN `modelid1` `modelid1` INT NOT NULL DEFAULT '0' AFTER `KillCredit2`,
  CHANGE COLUMN `modelid2` `modelid2` INT NOT NULL DEFAULT '0' AFTER `modelid1`,
  CHANGE COLUMN `modelid3` `modelid3` INT NOT NULL DEFAULT '0' AFTER `modelid2`,
  CHANGE COLUMN `modelid4` `modelid4` INT NOT NULL DEFAULT '0' AFTER `modelid3`;
