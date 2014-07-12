/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "rollback.h"
#include <fstream>
#include <list>
#include <sstream>
#include "log.h"
#include "mapnode.h"
#include "gamedef.h"
#include "nodedef.h"
#include "util/serialize.h"
#include "util/string.h"
#include "util/numeric.h"
#include "inventorymanager.h" // deserializing InventoryLocations
#include "sqlite3.h"
#include "filesys.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

#define POINTS_PER_NODE (16.0)

static std::string dbp;
static sqlite3* dbh                        = NULL;
static sqlite3_stmt* dbs_insert            = NULL;
static sqlite3_stmt* dbs_replace           = NULL;
static sqlite3_stmt* dbs_select            = NULL;
static sqlite3_stmt* dbs_select_range      = NULL;
static sqlite3_stmt* dbs_select_withActor  = NULL;
static sqlite3_stmt* dbs_knownActor_select = NULL;
static sqlite3_stmt* dbs_knownActor_insert = NULL;
static sqlite3_stmt* dbs_knownNode_select  = NULL;
static sqlite3_stmt* dbs_knownNode_insert  = NULL;

struct Stack {
	int node;
	int quantity;
};
struct ActionRow {
	int         id;
	int         actor;
	time_t      timestamp;
	int         type;
	std::string location, list;
	int         index, add;
	Stack       stack;
	int         nodeMeta;
	int         x, y, z;
	int         oldNode;
	int         oldParam1, oldParam2;
	std::string oldMeta;
	int         newNode;
	int         newParam1, newParam2;
	std::string newMeta;
	int         guessed;
};

struct Entity {
	int         id;
	std::string name;
};

typedef std::vector<Entity> Entities;

Entities KnownActors;
Entities KnownNodes;

void registerNewActor(int id, std::string name)
{
	Entity newActor;

	newActor.id   = id;
	newActor.name = name;

	KnownActors.push_back(newActor);

	//std::cout << "New actor registered: " << id << " | " << name << std::endl;
}

void registerNewNode(int id, std::string name)
{
	Entity newNode;

	newNode.id   = id;
	newNode.name = name;

	KnownNodes.push_back(newNode);

	//std::cout << "New node registered: " << id << " | " << name << std::endl;
}

int getActorId(std::string name)
{
	Entities::const_iterator iter;

	for (iter = KnownActors.begin(); iter != KnownActors.end(); ++iter)
		if (iter->name == name) {
			return iter->id;
		}

	sqlite3_reset(dbs_knownActor_insert);
	sqlite3_bind_text(dbs_knownActor_insert, 1, name.c_str(), -1, NULL);
	sqlite3_step(dbs_knownActor_insert);

	int id = sqlite3_last_insert_rowid(dbh);

	//std::cout << "Actor ID insert returns " << insert << std::endl;

	registerNewActor(id, name);

	return id;
}

int getNodeId(std::string name)
{
	Entities::const_iterator iter;

	for (iter = KnownNodes.begin(); iter != KnownNodes.end(); ++iter)
		if (iter->name == name) {
			return iter->id;
		}

	sqlite3_reset(dbs_knownNode_insert);
	sqlite3_bind_text(dbs_knownNode_insert, 1, name.c_str(), -1, NULL);
	sqlite3_step(dbs_knownNode_insert);

	int id = sqlite3_last_insert_rowid(dbh);

	registerNewNode(id, name);

	return id;
}

const char * getActorName(int id)
{
	Entities::const_iterator iter;

	//std::cout << "getActorName of id " << id << std::endl;

	for (iter = KnownActors.begin(); iter != KnownActors.end(); ++iter)
		if (iter->id == id) {
			return iter->name.c_str();
		}

	return "";
}

const char * getNodeName(int id)
{
	Entities::const_iterator iter;

	//std::cout << "getNodeName of id " << id << std::endl;

	for (iter = KnownNodes.begin(); iter != KnownNodes.end(); ++iter)
		if (iter->id == id) {
			return iter->name.c_str();
		}

	return "";
}

Stack getStackFromString(std::string text)
{
	Stack stack;

	size_t off = text.find_last_of(" ");

	stack.node     = getNodeId(text.substr(0, off));
	stack.quantity = atoi(text.substr(off + 1).c_str());

	return stack;
}

std::string getStringFromStack(Stack stack)
{
	std::string text;

	text.append(getNodeName(stack.node));
	text.append(" ");
	text.append(itos(stack.quantity));

	return text;
}

bool SQL_createDatabase(void)
{
	infostream << "CreateDB:" << dbp << std::endl;

	int dbs = sqlite3_exec(dbh,
		"CREATE TABLE IF NOT EXISTS `actor` ("
			"`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
			"`name` TEXT NOT NULL);"
			"CREATE TABLE IF NOT EXISTS `node` ("
			"`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
			"`name` TEXT NOT NULL);"
		"CREATE TABLE IF NOT EXISTS `action` ("
			"`id` INTEGER PRIMARY KEY AUTOINCREMENT,"
			"`actor` INTEGER NOT NULL,"
			"`timestamp` TIMESTAMP NOT NULL,"
			"`type` INTEGER NOT NULL,"
			"`list` TEXT,"
			"`index` INTEGER,"
			"`add` INTEGER,"
			"`stackNode` INTEGER,"
			"`stackQuantity` INTEGER,"
			"`nodeMeta` INTEGER,"
			"`x` INT,"
			"`y` INT,"
			"`z` INT,"
			"`oldNode` INTEGER,"
			"`oldParam1` INTEGER,"
			"`oldParam2` INTEGER,"
			"`oldMeta` TEXT,"
			"`newNode` INTEGER,"
			"`newParam1` INTEGER,"
			"`newParam2` INTEGER,"
			"`newMeta` TEXT,"
			"`guessedActor` INTEGER,"
			"FOREIGN KEY (`actor`)   REFERENCES `actor`(`id`),"
			"FOREIGN KEY (`oldNode`) REFERENCES `node`(`id`),"
			"FOREIGN KEY (`newNode`) REFERENCES `node`(`id`));"
		"CREATE INDEX IF NOT EXISTS `actionActor` ON `action`(`actor`);"
		"CREATE INDEX IF NOT EXISTS `actionTimestamp` ON `action`(`timestamp`);",
		NULL, NULL, NULL);
	if (dbs == SQLITE_ABORT) {
		throw FileNotGoodException("Could not create sqlite3 database structure");
	} else if (dbs != 0) {
		throw FileNotGoodException("SQL Rollback: Exec statement to create table structure returned a non-zero value");
	} else {
		infostream << "SQL Rollback: SQLite3 database structure was created" << std::endl;
	}

	return true;
}
void SQL_databaseCheck(void)
{
	if (dbh) {
		return;
	}

	infostream << "Database connection setup" << std::endl;

	bool needsCreate = !fs::PathExists(dbp);
	int  dbo = sqlite3_open_v2(dbp.c_str(), &dbh, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
	                           NULL);

	if (dbo != SQLITE_OK) {
		infostream << "SQLROLLBACK: SQLite3 database failed to open: "
		           << sqlite3_errmsg(dbh) << std::endl;
		throw FileNotGoodException("Cannot open database file");
	}

	if (needsCreate) {
		SQL_createDatabase();
	}

	int dbr;

	dbr = sqlite3_prepare_v2(dbh,
		"INSERT INTO `action` ("
		"	`actor`, `timestamp`, `type`,"
		"	`list`, `index`, `add`, `stackNode`, `stackQuantity`, `nodeMeta`,"
		"	`x`, `y`, `z`,"
		"	`oldNode`, `oldParam1`, `oldParam2`, `oldMeta`,"
		"	`newNode`, `newParam1`, `newParam2`, `newMeta`,"
		"	`guessedActor`"
		") VALUES ("
		"	?, ?, ?,"
		"	?, ?, ?, ?, ?, ?,"
		"	?, ?, ?,"
		"	?, ?, ?, ?,"
		"	?, ?, ?, ?,"
		"	?"
		");",
		-1, &dbs_insert, NULL);

	if (dbr != SQLITE_OK) {
		throw FileNotGoodException(sqlite3_errmsg(dbh));
	}

	dbr = sqlite3_prepare_v2(dbh,
		"REPLACE INTO `action` ("
		"	`actor`, `timestamp`, `type`,"
		"	`list`, `index`, `add`, `stackNode`, `stackQuantity`, `nodeMeta`,"
		"	`x`, `y`, `z`,"
		"	`oldNode`, `oldParam1`, `oldParam2`, `oldMeta`,"
		"	`newNode`, `newParam1`, `newParam2`, `newMeta`,"
		"	`guessedActor`, `id`"
		") VALUES ("
		"	?, ?, ?,"
		"	?, ?, ?, ?, ?, ?,"
		"	?, ?, ?,"
		"	?, ?, ?, ?,"
		"	?, ?, ?, ?,"
		"	?, ?"
		");",
		-1, &dbs_replace, NULL);

	if (dbr != SQLITE_OK) {
		throw FileNotGoodException(sqlite3_errmsg(dbh));
	}

	dbr = sqlite3_prepare_v2(dbh,
		"SELECT"
		"	`actor`, `timestamp`, `type`,"
		"	`list`, `index`, `add`, `stackNode`, `stackQuantity`, `nodemeta`,"
		"	`x`, `y`, `z`,"
		"	`oldNode`, `oldParam1`, `oldParam2`, `oldMeta`,"
		"	`newNode`, `newParam1`, `newParam2`, `newMeta`,"
		"	`guessedActor`"
		" FROM	`action`"
		" WHERE	`timestamp` >= ?"
		" ORDER BY `timestamp` DESC, `id` DESC",
		-1, &dbs_select, NULL);
	if (dbr != SQLITE_OK) {
		throw FileNotGoodException(itos(dbr).c_str());
	}

	dbr = sqlite3_prepare_v2(dbh,
		"SELECT"
		"	`actor`, `timestamp`, `type`,"
		"	`list`, `index`, `add`, `stackNode`, `stackQuantity`, `nodemeta`,"
		"	`x`, `y`, `z`,"
		"	`oldNode`, `oldParam1`, `oldParam2`, `oldMeta`,"
		"	`newNode`, `newParam1`, `newParam2`, `newMeta`,"
		"	`guessedActor`"
		" FROM	`action`"
		" WHERE	`timestamp` >= ?"
		" AND   `x` IS NOT NULL"
		" AND   `y` IS NOT NULL"
		" AND   `z` IS NOT NULL"
		" AND   ABS(`x` - ?) <= ?"
		" AND   ABS(`y` - ?) <= ?"
		" AND   ABS(`z` - ?) <= ?"
		" ORDER BY `timestamp` DESC, `id` DESC"
		" LIMIT 0,?",
		-1, &dbs_select_range, NULL);
	if (dbr != SQLITE_OK) {
		throw FileNotGoodException(itos(dbr).c_str());
	}

	dbr = sqlite3_prepare_v2(dbh,
		"SELECT"
		"	`actor`, `timestamp`, `type`,"
		"	`list`, `index`, `add`, `stackNode`, `stackQuantity`, `nodemeta`,"
		"	`x`, `y`, `z`,"
		"	`oldNode`, `oldParam1`, `oldParam2`, `oldMeta`,"
		"	`newNode`, `newParam1`, `newParam2`, `newMeta`,"
		"	`guessedActor`"
		" FROM	`action`"
		" WHERE	`timestamp` >= ?"
		" AND   `actor` = ?"
		" ORDER BY `timestamp` DESC, `id` DESC",
		-1, &dbs_select_withActor, NULL);
	if (dbr != SQLITE_OK) {
		throw FileNotGoodException(itos(dbr).c_str());
	}

	dbr = sqlite3_prepare_v2(dbh, "SELECT `id`, `name` FROM `actor`", -1,
	                         &dbs_knownActor_select, NULL);
	if (dbr != SQLITE_OK) {
		throw FileNotGoodException(itos(dbr).c_str());
	}

	dbr = sqlite3_prepare_v2(dbh, "INSERT INTO `actor` (`name`) VALUES (?)", -1,
	                         &dbs_knownActor_insert, NULL);
	if (dbr != SQLITE_OK) {
		throw FileNotGoodException(itos(dbr).c_str());
	}

	dbr = sqlite3_prepare_v2(dbh, "SELECT `id`, `name` FROM `node`", -1,
	                         &dbs_knownNode_select, NULL);
	if (dbr != SQLITE_OK) {
		throw FileNotGoodException(itos(dbr).c_str());
	}

	dbr = sqlite3_prepare_v2(dbh, "INSERT INTO `node` (`name`) VALUES (?)", -1,
	                         &dbs_knownNode_insert, NULL);
	if (dbr != SQLITE_OK) {
		throw FileNotGoodException(itos(dbr).c_str());
	}

	infostream << "SQL prepared statements setup correctly" << std::endl;

	int select;

	sqlite3_reset(dbs_knownActor_select);
	while (SQLITE_ROW == (select = sqlite3_step(dbs_knownActor_select)))
		registerNewActor(
		        sqlite3_column_int(dbs_knownActor_select, 0),
		        reinterpret_cast<const char *>(sqlite3_column_text(dbs_knownActor_select, 1))
		);

	sqlite3_reset(dbs_knownNode_select);
	while (SQLITE_ROW == (select = sqlite3_step(dbs_knownNode_select)))
		registerNewNode(
		        sqlite3_column_int(dbs_knownNode_select, 0),
		        reinterpret_cast<const char *>(sqlite3_column_text(dbs_knownNode_select, 1))
		);

	return;
}
bool SQL_registerRow(ActionRow row)
{
	SQL_databaseCheck();

	sqlite3_stmt * dbs_do = (row.id) ? dbs_replace : dbs_insert;

	/*
	std::cout
		<< (row.id? "Replacing": "Inserting")
		<< " ActionRow" << std::endl;
	*/
	sqlite3_reset(dbs_do);

	int bind [22], ii = 0;
	bool nodeMeta = false;

	bind[ii++] = sqlite3_bind_int(dbs_do, 1, row.actor);
	bind[ii++] = sqlite3_bind_int64(dbs_do, 2, row.timestamp);
	bind[ii++] = sqlite3_bind_int(dbs_do, 3, row.type);

	if (row.type == RollbackAction::TYPE_MODIFY_INVENTORY_STACK) {
		std::string loc		= row.location;
		std::string locType	= loc.substr(0, loc.find(":"));
		nodeMeta = (locType == "nodemeta");

		bind[ii++] = sqlite3_bind_text(dbs_do, 4, row.list.c_str(), row.list.size(), NULL);
		bind[ii++] = sqlite3_bind_int(dbs_do, 5, row.index);
		bind[ii++] = sqlite3_bind_int(dbs_do, 6, row.add);
		bind[ii++] = sqlite3_bind_int(dbs_do, 7, row.stack.node);
		bind[ii++] = sqlite3_bind_int(dbs_do, 8, row.stack.quantity);
		bind[ii++] = sqlite3_bind_int(dbs_do, 9, (int) nodeMeta);

		if (nodeMeta) {
			std::string x, y, z;
			int	l, r;
			l = loc.find(':') + 1;
			r = loc.find(',');
			x = loc.substr(l, r - l);
			l = r + 1;
			r = loc.find(',', l);
			y = loc.substr(l, r - l);
			z = loc.substr(r + 1);
			bind[ii++] = sqlite3_bind_int(dbs_do, 10, atoi(x.c_str()));
			bind[ii++] = sqlite3_bind_int(dbs_do, 11, atoi(y.c_str()));
			bind[ii++] = sqlite3_bind_int(dbs_do, 12, atoi(z.c_str()));
		}
	} else {
		bind[ii++] = sqlite3_bind_null(dbs_do, 4);
		bind[ii++] = sqlite3_bind_null(dbs_do, 5);
		bind[ii++] = sqlite3_bind_null(dbs_do, 6);
		bind[ii++] = sqlite3_bind_null(dbs_do, 7);
		bind[ii++] = sqlite3_bind_null(dbs_do, 8);
		bind[ii++] = sqlite3_bind_null(dbs_do, 9);
	}

	if (row.type == RollbackAction::TYPE_SET_NODE) {
		bind[ii++] = sqlite3_bind_int(dbs_do, 10, row.x);
		bind[ii++] = sqlite3_bind_int(dbs_do, 11, row.y);
		bind[ii++] = sqlite3_bind_int(dbs_do, 12, row.z);
		bind[ii++] = sqlite3_bind_int(dbs_do, 13, row.oldNode);
		bind[ii++] = sqlite3_bind_int(dbs_do, 14, row.oldParam1);
		bind[ii++] = sqlite3_bind_int(dbs_do, 15, row.oldParam2);
		bind[ii++] = sqlite3_bind_text(dbs_do, 16, row.oldMeta.c_str(), row.oldMeta.size(), NULL);
		bind[ii++] = sqlite3_bind_int(dbs_do, 17, row.newNode);
		bind[ii++] = sqlite3_bind_int(dbs_do, 18, row.newParam1);
		bind[ii++] = sqlite3_bind_int(dbs_do, 19, row.newParam2);
		bind[ii++] = sqlite3_bind_text(dbs_do, 20, row.newMeta.c_str(), row.newMeta.size(), NULL);
		bind[ii++] = sqlite3_bind_int(dbs_do, 21, row.guessed ? 1 : 0);
	} else {
		if (!nodeMeta) {
			bind[ii++] = sqlite3_bind_null(dbs_do, 10);
			bind[ii++] = sqlite3_bind_null(dbs_do, 11);
			bind[ii++] = sqlite3_bind_null(dbs_do, 12);
		}
		bind[ii++] = sqlite3_bind_null(dbs_do, 13);
		bind[ii++] = sqlite3_bind_null(dbs_do, 14);
		bind[ii++] = sqlite3_bind_null(dbs_do, 15);
		bind[ii++] = sqlite3_bind_null(dbs_do, 16);
		bind[ii++] = sqlite3_bind_null(dbs_do, 17);
		bind[ii++] = sqlite3_bind_null(dbs_do, 18);
		bind[ii++] = sqlite3_bind_null(dbs_do, 19);
		bind[ii++] = sqlite3_bind_null(dbs_do, 20);
		bind[ii++] = sqlite3_bind_null(dbs_do, 21);
	}

	if (row.id) {
		bind[ii++] = sqlite3_bind_int(dbs_do, 22, row.id);
	}

	for (ii = 0; ii < 20; ++ii)
		if (bind[ii] != SQLITE_OK)
			infostream
			                << "WARNING: failed to bind param " << ii + 1
			                << " when inserting an entry in table setnode" << std::endl;

	/*
	std::cout << "========DB-WRITTEN==========" << std::endl;
	std::cout << "id:        " << row.id << std::endl;
	std::cout << "actor:     " << row.actor << std::endl;
	std::cout << "time:      " << row.timestamp << std::endl;
	std::cout << "type:      " << row.type << std::endl;
	if (row.type == RollbackAction::TYPE_MODIFY_INVENTORY_STACK)
	{
		std::cout << "Location:   " << row.location << std::endl;
		std::cout << "List:       " << row.list << std::endl;
		std::cout << "Index:      " << row.index << std::endl;
		std::cout << "Add:        " << row.add << std::endl;
		std::cout << "Stack:      " << row.stack << std::endl;
	}
	if (row.type == RollbackAction::TYPE_SET_NODE)
	{
		std::cout << "x:         " << row.x << std::endl;
		std::cout << "y:         " << row.y << std::endl;
		std::cout << "z:         " << row.z << std::endl;
		std::cout << "oldNode:   " << row.oldNode << std::endl;
		std::cout << "oldParam1: " << row.oldParam1 << std::endl;
		std::cout << "oldParam2: " << row.oldParam2 << std::endl;
		std::cout << "oldMeta:   " << row.oldMeta << std::endl;
		std::cout << "newNode:   " << row.newNode << std::endl;
		std::cout << "newParam1: " << row.newParam1 << std::endl;
		std::cout << "newParam2: " << row.newParam2 << std::endl;
		std::cout << "newMeta:   " << row.newMeta << std::endl;
		std::cout << "DESERIALIZE" << row.newMeta.c_str() << std::endl;
		std::cout << "guessed:   " << row.guessed << std::endl;
	}
	*/

	int written = sqlite3_step(dbs_do);

	return written == SQLITE_DONE;

	//if  (written != SQLITE_DONE)
	//	 std::cout << "WARNING: rollback action not written: " << sqlite3_errmsg(dbh) << std::endl;
	//else std::cout << "Action correctly inserted via SQL" << std::endl;
}
std::list<ActionRow> actionRowsFromSelect(sqlite3_stmt* stmt)
{
	std::list<ActionRow> rows;
	const unsigned char * text;
	size_t size;

	while (SQLITE_ROW == sqlite3_step(stmt)) {
		ActionRow row;

		row.actor     = sqlite3_column_int(stmt, 0);
		row.timestamp = sqlite3_column_int64(stmt, 1);
		row.type      = sqlite3_column_int(stmt, 2);

		if (row.type == RollbackAction::TYPE_MODIFY_INVENTORY_STACK) {
			text               = sqlite3_column_text(stmt, 3);
			size               = sqlite3_column_bytes(stmt, 3);
			row.list           = std::string(reinterpret_cast<const char*>(text), size);
			row.index          = sqlite3_column_int(stmt, 4);
			row.add            = sqlite3_column_int(stmt, 5);
			row.stack.node     = sqlite3_column_int(stmt, 6);
			row.stack.quantity = sqlite3_column_int(stmt, 7);
			row.nodeMeta       = sqlite3_column_int(stmt, 8);
		}

		if (row.type == RollbackAction::TYPE_SET_NODE || row.nodeMeta) {
			row.x = sqlite3_column_int(stmt,  9);
			row.y = sqlite3_column_int(stmt, 10);
			row.z = sqlite3_column_int(stmt, 11);
		}

		if (row.type == RollbackAction::TYPE_SET_NODE) {
			row.oldNode		= sqlite3_column_int(stmt, 12);
			row.oldParam1 = sqlite3_column_int(stmt, 13);
			row.oldParam2 = sqlite3_column_int(stmt, 14);
			text          = sqlite3_column_text(stmt, 15);
			size          = sqlite3_column_bytes(stmt, 15);
			row.oldMeta   = std::string(reinterpret_cast<const char*>(text), size);
			row.newNode   = sqlite3_column_int(stmt, 16);
			row.newParam1 = sqlite3_column_int(stmt, 17);
			row.newParam2 = sqlite3_column_int(stmt, 18);
			text          = sqlite3_column_text(stmt, 19);
			size          = sqlite3_column_bytes(stmt, 19);
			row.newMeta   = std::string(reinterpret_cast<const char*>(text), size);
			row.guessed   = sqlite3_column_int(stmt, 20);
		}

		row.location = row.nodeMeta ? "nodemeta:" : getActorName(row.actor);

		if (row.nodeMeta) {
			row.location.append(itos(row.x));
			row.location.append(",");
			row.location.append(itos(row.y));
			row.location.append(",");
			row.location.append(itos(row.z));
		}

		/*
		std::cout << "=======SELECTED==========" << "\n";
		std::cout << "Actor: " << row.actor << "\n";
		std::cout << "Timestamp: " << row.timestamp << "\n";

		if (row.type == RollbackAction::TYPE_MODIFY_INVENTORY_STACK)
		{
			std::cout << "list: " << row.list << "\n";
			std::cout << "index: " << row.index << "\n";
			std::cout << "add: " << row.add << "\n";
			std::cout << "stackNode: " << row.stack.node << "\n";
			std::cout << "stackQuantity: " << row.stack.quantity << "\n";
			if (row.nodeMeta)
			{
				std::cout << "X: " << row.x << "\n";
				std::cout << "Y: " << row.y << "\n";
				std::cout << "Z: " << row.z << "\n";
			}
			std::cout << "Location: " << row.location << "\n";
		}
			else
		{
			std::cout << "X: " << row.x << "\n";
			std::cout << "Y: " << row.y << "\n";
			std::cout << "Z: " << row.z << "\n";
			std::cout << "oldNode: " << row.oldNode << "\n";
			std::cout << "oldParam1: " << row.oldParam1 << "\n";
			std::cout << "oldParam2: " << row.oldParam2 << "\n";
			std::cout << "oldMeta: " << row.oldMeta << "\n";
			std::cout << "newNode: " << row.newNode << "\n";
			std::cout << "newParam1: " << row.newParam1 << "\n";
			std::cout << "newParam2: " << row.newParam2 << "\n";
			std::cout << "newMeta: " << row.newMeta << "\n";
			std::cout << "guessed: " << row.guessed << "\n";
		}
		*/

		rows.push_back(row);
	}

	return rows;
}
ActionRow actionRowFromRollbackAction(RollbackAction action)
{
	ActionRow row;

	row.id        = 0;
	row.actor     = getActorId(action.actor);
	row.timestamp = action.unix_time;
	row.type      = action.type;

	if (row.type == RollbackAction::TYPE_MODIFY_INVENTORY_STACK) {
		row.location = action.inventory_location;
		row.list     = action.inventory_list;
		row.index    = action.inventory_index;
		row.add      = action.inventory_add;
		row.stack    = getStackFromString(action.inventory_stack);
	} else {
		row.x         = action.p.X;
		row.y         = action.p.Y;
		row.z         = action.p.Z;
		row.oldNode   = getNodeId(action.n_old.name);
		row.oldParam1 = action.n_old.param1;
		row.oldParam2 = action.n_old.param2;
		row.oldMeta   = action.n_old.meta;
		row.newNode   = getNodeId(action.n_new.name);
		row.newParam1 = action.n_new.param1;
		row.newParam2 = action.n_new.param2;
		row.newMeta   = action.n_new.meta;
		row.guessed   = action.actor_is_guess;
	}

	return row;
}
std::list<RollbackAction> rollbackActionsFromActionRows(std::list<ActionRow> rows)
{
	std::list<RollbackAction> actions;
	std::list<ActionRow>::const_iterator it;

	for (it = rows.begin(); it != rows.end(); ++it) {
		RollbackAction action;
		action.actor     = (it->actor) ? getActorName(it->actor) : "";
		action.unix_time = it->timestamp;
		action.type      = static_cast<RollbackAction::Type>(it->type);

		switch (action.type) {
		case RollbackAction::TYPE_MODIFY_INVENTORY_STACK:

			action.inventory_location = it->location.c_str();
			action.inventory_list     = it->list;
			action.inventory_index    = it->index;
			action.inventory_add      = it->add;
			action.inventory_stack    = getStringFromStack(it->stack);
			break;

		case RollbackAction::TYPE_SET_NODE:

			action.p            = v3s16(it->x, it->y, it->z);
			action.n_old.name   = getNodeName(it->oldNode);
			action.n_old.param1 = it->oldParam1;
			action.n_old.param2 = it->oldParam2;
			action.n_old.meta   = it->oldMeta;
			action.n_new.name   = getNodeName(it->newNode);
			action.n_new.param1 = it->newParam1;
			action.n_new.param2 = it->newParam2;
			action.n_new.meta   = it->newMeta;
			break;

		default:

			throw ("W.T.F.");
			break;
		}

		actions.push_back(action);
	}

	return actions;
}

std::list<ActionRow> SQL_getRowsSince(time_t firstTime, std::string actor = "")
{
	sqlite3_stmt *dbs_stmt = (!actor.length()) ? dbs_select : dbs_select_withActor;
	sqlite3_reset(dbs_stmt);
	sqlite3_bind_int64(dbs_stmt, 1, firstTime);

	if (actor.length()) {
		sqlite3_bind_int(dbs_stmt, 2, getActorId(actor));
	}

	return actionRowsFromSelect(dbs_stmt);
}

std::list<ActionRow> SQL_getRowsSince_range(time_t firstTime, v3s16 p, int range,
                int limit)
{
	sqlite3_stmt *stmt = dbs_select_range;
	sqlite3_reset(stmt);

	sqlite3_bind_int64(stmt, 1, firstTime);
	sqlite3_bind_int(stmt, 2, (int) p.X);
	sqlite3_bind_int(stmt, 3, range);
	sqlite3_bind_int(stmt, 4, (int) p.Y);
	sqlite3_bind_int(stmt, 5, range);
	sqlite3_bind_int(stmt, 6, (int) p.Z);
	sqlite3_bind_int(stmt, 7, range);
	sqlite3_bind_int(stmt, 8, limit);

	return actionRowsFromSelect(stmt);
}

std::list<RollbackAction> SQL_getActionsSince_range(time_t firstTime, v3s16 p, int range,
                int limit)
{
	std::list<ActionRow> rows = SQL_getRowsSince_range(firstTime, p, range, limit);

	return rollbackActionsFromActionRows(rows);
}

std::list<RollbackAction> SQL_getActionsSince(time_t firstTime, std::string actor = "")
{
	std::list<ActionRow> rows = SQL_getRowsSince(firstTime, actor);
	return rollbackActionsFromActionRows(rows);
}

void TXT_migrate(std::string filepath)
{
	std::cout << "Migrating from rollback.txt to rollback.sqlite" << std::endl;
	SQL_databaseCheck();

	std::ifstream fh(filepath.c_str(), std::ios::in | std::ios::ate);
	if (!fh.good()) {
		throw ("DIE");
	}

	std::streampos filesize = fh.tellg();

	if (filesize > 10) {
		fh.seekg(0);

		std::string bit;
		int i   = 0;
		int id  = 1;
		time_t t   = 0;
		do {
			ActionRow row;

			row.id = id;

			// Get the timestamp
			std::getline(fh, bit, ' ');
			bit = trim(bit);
			if (!atoi(trim(bit).c_str())) {
				std::getline(fh, bit);
				continue;
			}
			row.timestamp = atoi(bit.c_str());

			// Get the actor
			row.actor = getActorId(trim(deSerializeJsonString(fh)));

			// Get the action type
			std::getline(fh, bit, '[');
			std::getline(fh, bit, ' ');

			if (bit == "modify_inventory_stack") {
				row.type = RollbackAction::TYPE_MODIFY_INVENTORY_STACK;
			}

			if (bit == "set_node") {
				row.type = RollbackAction::TYPE_SET_NODE;
			}

			if (row.type == RollbackAction::TYPE_MODIFY_INVENTORY_STACK) {
				row.location = trim(deSerializeJsonString(fh));
				std::getline(fh, bit, ' ');
				row.list     = trim(deSerializeJsonString(fh));
				std::getline(fh, bit, ' ');
				std::getline(fh, bit, ' ');
				row.index    = atoi(trim(bit).c_str());
				std::getline(fh, bit, ' ');
				row.add      = (int)(trim(bit) == "add");
				row.stack    = getStackFromString(trim(deSerializeJsonString(fh)));
				std::getline(fh, bit);
			} else if (row.type == RollbackAction::TYPE_SET_NODE) {
				std::getline(fh, bit, '(');
				std::getline(fh, bit, ',');
				row.x       = atoi(trim(bit).c_str());
				std::getline(fh, bit, ',');
				row.y       = atoi(trim(bit).c_str());
				std::getline(fh, bit, ')');
				row.z       = atoi(trim(bit).c_str());
				std::getline(fh, bit, ' ');
				row.oldNode = getNodeId(trim(deSerializeJsonString(fh)));
				std::getline(fh, bit, ' ');
				std::getline(fh, bit, ' ');
				row.oldParam1 = atoi(trim(bit).c_str());
				std::getline(fh, bit, ' ');
				row.oldParam2 = atoi(trim(bit).c_str());
				row.oldMeta   = trim(deSerializeJsonString(fh));
				std::getline(fh, bit, ' ');
				row.newNode   = getNodeId(trim(deSerializeJsonString(fh)));
				std::getline(fh, bit, ' ');
				std::getline(fh, bit, ' ');
				row.newParam1 = atoi(trim(bit).c_str());
				std::getline(fh, bit, ' ');
				row.newParam2 = atoi(trim(bit).c_str());
				row.newMeta   = trim(deSerializeJsonString(fh));
				std::getline(fh, bit, ' ');
				std::getline(fh, bit, ' ');
				std::getline(fh, bit);
				row.guessed = (int)(trim(bit) == "actor_is_guess");
			}

			/*
			std::cout << "==========READ===========" << std::endl;
			std::cout << "time:      " << row.timestamp << std::endl;
			std::cout << "actor:     " << row.actor << std::endl;
			std::cout << "type:      " << row.type << std::endl;
			if (row.type == RollbackAction::TYPE_MODIFY_INVENTORY_STACK)
			{
				std::cout << "Location:   " << row.location << std::endl;
				std::cout << "List:       " << row.list << std::endl;
				std::cout << "Index:      " << row.index << std::endl;
				std::cout << "Add:        " << row.add << std::endl;
				std::cout << "Stack:      " << row.stack << std::endl;
			}
			if (row.type == RollbackAction::TYPE_SET_NODE)
			{
				std::cout << "x:         " << row.x << std::endl;
				std::cout << "y:         " << row.y << std::endl;
				std::cout << "z:         " << row.z << std::endl;
				std::cout << "oldNode:   " << row.oldNode << std::endl;
				std::cout << "oldParam1: " << row.oldParam1 << std::endl;
				std::cout << "oldParam2: " << row.oldParam2 << std::endl;
				std::cout << "oldMeta:   " << row.oldMeta << std::endl;
				std::cout << "newNode:   " << row.newNode << std::endl;
				std::cout << "newParam1: " << row.newParam1 << std::endl;
				std::cout << "newParam2: " << row.newParam2 << std::endl;
				std::cout << "newMeta:   " << row.newMeta << std::endl;
				std::cout << "guessed:   " << row.guessed << std::endl;
			}
			*/

			if (i == 0) {
				t = time(0);
				sqlite3_exec(dbh, "BEGIN", NULL, NULL, NULL);
			}

			SQL_registerRow(row);
			++i;

			if (time(0) - t) {
				sqlite3_exec(dbh, "COMMIT", NULL, NULL, NULL);
				t = time(0) - t;
				std::cout
				                << " Done: " << (int)(((float) fh.tellg() / (float) filesize) * 100) << "%"
				                << " Speed: " << i / t << "/second     \r" << std::flush;
				i = 0;
			}

			++id;
		} while (!fh.eof() && fh.good());
	} else {
		errorstream << "Empty rollback log" << std::endl;
	}

	std::cout
	                << " Done: 100%                                   " << std::endl
	                << " Now you can delete the old rollback.txt file." << std::endl;
}

// Get nearness factor for subject's action for this action
// Return value: 0 = impossible, >0 = factor
static float getSuspectNearness(bool is_guess, v3s16 suspect_p, time_t suspect_t,
                                v3s16 action_p, time_t action_t)
{
	// Suspect cannot cause things in the past
	if (action_t < suspect_t) {
		return 0;        // 0 = cannot be
	}
	// Start from 100
	int f = 100;
	// Distance (1 node = -x points)
	f -= POINTS_PER_NODE * intToFloat(suspect_p, 1).getDistanceFrom(intToFloat(action_p, 1));
	// Time (1 second = -x points)
	f -= 1 * (action_t - suspect_t);
	// If is a guess, halve the points
	if (is_guess) {
		f *= 0.5;
	}
	// Limit to 0
	if (f < 0) {
		f = 0;
	}
	return f;
}
class RollbackManager: public IRollbackManager
{
public:
	// IRollbackManager interface
	void reportAction(const RollbackAction &action_) {
		// Ignore if not important
		if (!action_.isImportant(m_gamedef)) {
			return;
		}

		RollbackAction action = action_;
		action.unix_time = time(0);

		// Figure out actor
		action.actor = m_current_actor;
		action.actor_is_guess = m_current_actor_is_guess;

		if (action.actor.empty()) { // If actor is not known, find out suspect or cancel
			v3s16 p;
			if (!action.getPosition(&p)) {
				return;
			}

			action.actor = getSuspect(p, 83, 1);
			if (action.actor.empty()) {
				return;
			}

			action.actor_is_guess = true;
		}

		infostream
		                << "RollbackManager::reportAction():"
		                << " time=" << action.unix_time
		                << " actor=\"" << action.actor << "\""
		                << (action.actor_is_guess ? " (guess)" : "")
		                << " action=" << action.toString()
		                << std::endl;
		addAction(action);
	}

	std::string getActor() {
		return m_current_actor;
	}

	bool isActorGuess() {
		return m_current_actor_is_guess;
	}

	void setActor(const std::string &actor, bool is_guess) {
		m_current_actor = actor;
		m_current_actor_is_guess = is_guess;
	}

	std::string getSuspect(v3s16 p, float nearness_shortcut, float min_nearness) {
		if (m_current_actor != "") {
			return m_current_actor;
		}
		int cur_time = time(0);
		time_t first_time = cur_time - (100 - min_nearness);
		RollbackAction likely_suspect;
		float likely_suspect_nearness = 0;
		for (std::list<RollbackAction>::const_reverse_iterator
		     i = m_action_latest_buffer.rbegin();
		     i != m_action_latest_buffer.rend(); i++) {
			if (i->unix_time < first_time) {
				break;
			}
			if (i->actor == "") {
				continue;
			}
			// Find position of suspect or continue
			v3s16 suspect_p;
			if (!i->getPosition(&suspect_p)) {
				continue;
			}
			float f = getSuspectNearness(i->actor_is_guess, suspect_p,
			                             i->unix_time, p, cur_time);
			if (f >= min_nearness && f > likely_suspect_nearness) {
				likely_suspect_nearness = f;
				likely_suspect = *i;
				if (likely_suspect_nearness >= nearness_shortcut) {
					break;
				}
			}
		}
		// No likely suspect was found
		if (likely_suspect_nearness == 0) {
			return "";
		}
		// Likely suspect was found
		return likely_suspect.actor;
	}

	void flush() {
		infostream << "RollbackManager::flush()" << std::endl;

		sqlite3_exec(dbh, "BEGIN", NULL, NULL, NULL);

		std::list<RollbackAction>::const_iterator iter;

		for (iter  = m_action_todisk_buffer.begin();
		     iter != m_action_todisk_buffer.end();
		     iter++) {
			if (iter->actor == "") {
				continue;
			}

			SQL_registerRow(actionRowFromRollbackAction(*iter));
		}

		sqlite3_exec(dbh, "COMMIT", NULL, NULL, NULL);
		m_action_todisk_buffer.clear();
	}

	RollbackManager(const std::string &filepath, IGameDef *gamedef):
		m_filepath(filepath),
		m_gamedef(gamedef),
		m_current_actor_is_guess(false) {
		infostream
		                << "RollbackManager::RollbackManager(" << filepath << ")"
		                << std::endl;

		// Operate correctly in case of still being given rollback.txt as filepath
		std::string directory   =  filepath.substr(0, filepath.rfind(DIR_DELIM) + 1);
		std::string filenameOld =  filepath.substr(filepath.rfind(DIR_DELIM) + 1);
		std::string filenameNew = (filenameOld == "rollback.txt") ? "rollback.sqlite" :
		                          filenameOld;
		std::string filenameTXT	=  directory + "rollback.txt";
		std::string migratingFlag = filepath + ".migrating";

		infostream << "Directory: " << directory   << std::endl;
		infostream << "CheckFor:  " << filenameTXT << std::endl;
		infostream << "FileOld:   " << filenameOld << std::endl;
		infostream << "FileNew:   " << filenameNew << std::endl;

		dbp = directory + filenameNew;

		if ((fs::PathExists(filenameTXT) && fs::PathExists(migratingFlag)) ||
		    (fs::PathExists(filenameTXT) && !fs::PathExists(dbp))) {
			std::ofstream of(migratingFlag.c_str());
			TXT_migrate(filenameTXT);
			fs::DeleteSingleFileOrEmptyDirectory(migratingFlag);
		}

		SQL_databaseCheck();
	}
#define FINALIZE_STATEMENT(statement)                                          \
	if ( statement )                                                           \
		rc = sqlite3_finalize(statement);                                      \
	statement = NULL;                                                          \
	if ( rc != SQLITE_OK )                                                     \
		errorstream << "RollbackManager::~RollbackManager():"                  \
			<< "Failed to finalize: " << #statement << ": rc=" << rc << std::endl;

	~RollbackManager() {
		infostream << "RollbackManager::~RollbackManager()" << std::endl;
		flush();

		int rc = SQLITE_OK;

		FINALIZE_STATEMENT(dbs_insert)
		FINALIZE_STATEMENT(dbs_replace)
		FINALIZE_STATEMENT(dbs_select)
		FINALIZE_STATEMENT(dbs_select_range)
		FINALIZE_STATEMENT(dbs_select_withActor)
		FINALIZE_STATEMENT(dbs_knownActor_select)
		FINALIZE_STATEMENT(dbs_knownActor_insert)
		FINALIZE_STATEMENT(dbs_knownNode_select)
		FINALIZE_STATEMENT(dbs_knownNode_insert)

		if(dbh)
			rc = sqlite3_close(dbh);

		dbh = NULL;

		if (rc != SQLITE_OK) {
			errorstream << "RollbackManager::~RollbackManager(): "
					<< "Failed to close database: rc=" << rc << std::endl;
		}
	}

	void addAction(const RollbackAction &action) {
		m_action_todisk_buffer.push_back(action);
		m_action_latest_buffer.push_back(action);

		// Flush to disk sometimes
		if (m_action_todisk_buffer.size() >= 500) {
			flush();
		}
	}

	std::list<RollbackAction> getEntriesSince(time_t first_time) {
		infostream
		                << "RollbackManager::getEntriesSince(" << first_time << ")"
		                << std::endl;

		flush();

		std::list<RollbackAction> result = SQL_getActionsSince(first_time);

		return result;
	}

	std::list<RollbackAction> getNodeActors(v3s16 pos, int range, time_t seconds, int limit) {
		time_t cur_time = time(0);
		time_t first_time = cur_time - seconds;

		return SQL_getActionsSince_range(first_time, pos, range, limit);
	}

	std::list<RollbackAction> getRevertActions(const std::string &actor_filter,
	                time_t seconds) {
		infostream
		                << "RollbackManager::getRevertActions(" << actor_filter
		                << ", " << seconds << ")"
		                << std::endl;

		// Figure out time
		time_t cur_time = time(0);
		time_t first_time = cur_time - seconds;

		flush();

		std::list<RollbackAction> result = SQL_getActionsSince(first_time, actor_filter);

		return result;
	}
private:
	std::string m_filepath;
	IGameDef *m_gamedef;
	std::string m_current_actor;
	bool m_current_actor_is_guess;
	std::list<RollbackAction> m_action_todisk_buffer;
	std::list<RollbackAction> m_action_latest_buffer;
};

IRollbackManager *createRollbackManager(const std::string &filepath, IGameDef *gamedef)
{
	return new RollbackManager(filepath, gamedef);
}

