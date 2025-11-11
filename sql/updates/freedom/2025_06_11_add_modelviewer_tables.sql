
CREATE TABLE IF NOT EXISTS `modelviewer_tag` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `tag` varchar(25) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `modelviewer_model_to_tag` (
	`model_id` int(10) unsigned NOT NULL,
	`tag_id` INT(10) unsigned NOT NULL,
	`user_id` INT (10) UNSIGNED NOT NULL,

    PRIMARY KEY (`model_id`, `tag_id`, `user_id`),
  	CONSTRAINT `mmtt_model` FOREIGN KEY (`model_id`) REFERENCES `modelviewer_models` (`id`),
  	CONSTRAINT `mmtt_tag` FOREIGN KEY (`tag_id`) REFERENCES `modelviewer_tag` (`id`),
  	CONSTRAINT `mmtt_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `modelviewer_collection` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
	`user_id` INT (10) UNSIGNED NOT NULL,
	`name` VARCHAR(250) NOT NULL DEFAULT '',
     PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `modelviewer_model_to_collection` (
	`model_id` int(10) unsigned NOT NULL,
	`collection_id` INT(10) unsigned NOT NULL,
     PRIMARY KEY (`model_id`, `collection_id`),
  	CONSTRAINT `mmtc_model` FOREIGN KEY (`model_id`) REFERENCES `modelviewer_models` (`id`),
  	CONSTRAINT `mmtc_collection` FOREIGN KEY (`collection_id`) REFERENCES `modelviewer_collection` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
