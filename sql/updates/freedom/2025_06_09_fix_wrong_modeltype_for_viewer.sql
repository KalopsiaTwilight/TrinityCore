UPDATE `modelviewer_models`
SET `type` = 'wmo'
WHERE `type` != 'wmo'
AND fileName LIKE '%.wmo'