WITH set_faction_ids AS (
SELECT DISTINCT entryorguid
FROM smart_scripts
WHERE ACTION_TYPE IN (2)
)
DELETE FROM smart_scripts
WHERE entryorguid IN (
SELECT * FROM set_faction_ids
)
