WITH spawning_ids AS (
SELECT DISTINCT entryorguid
FROM smart_scripts
WHERE ACTION_TYPE IN (107,131, 12)
)
DELETE FROM smart_scripts
WHERE entryorguid IN (
SELECT * FROM spawning_ids
)
