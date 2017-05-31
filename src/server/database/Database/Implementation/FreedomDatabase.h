/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FREEDOMDATABASE_H
#define _FREEDOMDATABASE_H

#include "MySQLConnection.h"

enum FreedomDatabaseStatements
{
    /*  Naming standard for defines:
        {DB}_{SEL/INS/UPD/DEL/REP}_{Summary of data changed}
        When updating more than one field, consider looking at the calling function
        name for a suiting suffix.
    */
    // SELECTS
	FREEDOM_SEL_PUBLIC_TELE,
    FREEDOM_SEL_PRIVATE_TELE,
    FREEDOM_SEL_PUBLIC_SPELL,
    FREEDOM_SEL_MORPHS,
    FREEDOM_SEL_ITEMTEMPLATEEXTRA,
    FREEDOM_SEL_GAMEOBJECTEXTRA,
    FREEDOM_SEL_GAMEOBJECTEXTRA_TEMPLATE,
    FREEDOM_SEL_CREATUREEXTRA,
    FREEDOM_SEL_CREATUREEXTRA_TEMPLATE,
    FREEDOM_SEL_ITEMTEMPLATEEXTRA_LOOKUP,
    FREEDOM_SEL_CHAR_PET_EXTRA_BY_ID,

    // INSERTS
    FREEDOM_INS_PUBLIC_TELE,
    FREEDOM_INS_PRIVATE_TELE,
    FREEDOM_INS_PUBLIC_SPELL,
    FREEDOM_INS_MORPHS,

    // DELETIONS
    FREEDOM_DEL_PUBLIC_TELE_NAME,
    FREEDOM_DEL_PRIVATE_TELE_NAME,
    FREEDOM_DEL_PUBLIC_SPELL_ID,
    FREEDOM_DEL_MORPHS_NAME,
    FREEDOM_DEL_GAMEOBJECTEXTRA,
    FREEDOM_DEL_CREATUREEXTRA,
    FREEDOM_DEL_CHAR_EXTRA,
    FREEDOM_DEL_CHAR_MORPHS,
    FREEDOM_DEL_CHAR_PET_BY_OWNER,
    FREEDOM_DEL_CHAR_PET_BY_ID,

    // UPDATES
    FREEDOM_UPD_ITEMTEMPLATEEXTRA,
    FREEDOM_UPD_GAMEOBJECTEXTRA_TEMPLATE,
    FREEDOM_UPD_CREATUREEXTRA_TEMPLATE,
    FREEDOM_UPD_CHAR_PET_EXTRA,

    // REPLACES
    FREEDOM_REP_GAMEOBJECTEXTRA,
    FREEDOM_REP_CREATUREEXTRA,

    MAX_FREEDOMDATABASE_STATEMENTS
};

class TC_DATABASE_API FreedomDatabaseConnection : public MySQLConnection
{
public:
    typedef FreedomDatabaseStatements Statements;

    //- Constructors for sync and async connections
    FreedomDatabaseConnection(MySQLConnectionInfo& connInfo);
    FreedomDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* q, MySQLConnectionInfo& connInfo);
    ~FreedomDatabaseConnection();

    //- Loads database type specific prepared statements
    void DoPrepareStatements() override;
};

#endif
