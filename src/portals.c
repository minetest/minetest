/* a database for portals alone? bah! this could use a shared database
   for generally persistent objects, but should avoid using the map 
   database for speed */

#include "portals.h"

#include "sqlite3.h"
#include <lualib.h>
#include <lauxlib.h>
#include <assert.h>

#include <stdlib.h>

#define true 1
#define false 0 // :p

sqlite3* database = NULL;

sqlite3_stmt* findOtherSide = NULL;
sqlite3_stmt* setThisSide = NULL;
sqlite3_stmt* maxPortal = NULL;
sqlite3_stmt* insertPortal = NULL;

sqlite3_stmt* begin = NULL;
sqlite3_stmt* rollback = NULL;
sqlite3_stmt* commit = NULL;

static int l_portal_setup(lua_State *L) {
  if(database) return 0;
  const char* path = lua_tostring(L,1);
  if(path==NULL) {
    perror("Derrrrrrrp");
    exit(23);
    return 0;
  }
  fprintf(stderr,"Database at %s\n",path);
  lua_pop(L, 1);
  int res = sqlite3_open_v2(path,&database,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                            NULL);
  if(res != SQLITE_OK) {
    perror("Couldn't database");
    exit(23);
    return 0;
  }

  assert(database);
  assert(SQLITE_OK == 
         sqlite3_exec(database,
                      "CREATE TABLE IF NOT EXISTS portals "
                      "(id INTEGER PRIMARY KEY,"
                      " otherEnd INTEGER,"
                      " x REAL,"
                      " y REAL,"
                      " z REAL)",NULL,NULL,NULL));

  sqlite3_extended_result_codes(database,true);

#define PREPARE(stmt,name)                                              \
  res = sqlite3_prepare_v2(database,stmt,-1,&name,NULL);                \
  if(res != SQLITE_OK) {                                                \
    fprintf(stderr,"PRep failed %s %s\n",stmt,sqlite3_errmsg(database)); \
    sqlite3_close(database);                                            \
    database = NULL;                                                    \
    exit(23);                                                           \
  }                                                                     
  
  PREPARE("SELECT other.x,other.y,other.z FROM portals "
          "INNER JOIN portals AS other "
          "ON other.id = portals.otherEnd "
          "WHERE portals.id = ?",
          findOtherSide);
  PREPARE("UPDATE portals SET x = ?, y = ?, z = ? WHERE id = ?",
          setThisSide);
  PREPARE("SELECT coalesce(MAX(id),0) FROM portals",
          maxPortal);
  PREPARE("INSERT INTO portals (id,otherEnd) VALUES (?,?)",
          insertPortal);
  PREPARE("BEGIN IMMEDIATE",begin);
  PREPARE("ROLLBACK",rollback);
  PREPARE("COMMIT",commit);
#undef PREPARE

  lua_pushboolean(L,true);
  return 1;
}

static int l_portal_find_otherside(lua_State *L) {
  sqlite3_int64 id = lua_tonumber(L, 1);
  if(id < 0) {
    luaL_typerror(L, 1, "portal id");
    return 0;
  }
  lua_pop(L, 1);
  sqlite3_reset(findOtherSide);
  sqlite3_bind_int64(findOtherSide,1,id);
  int res = sqlite3_step(findOtherSide);  
  if(res != SQLITE_ROW || sqlite3_column_count(findOtherSide) != 3) {
    sqlite3_reset(findOtherSide);
    return 0;
  }
  lua_pushnumber(L,sqlite3_column_double(findOtherSide,0));
  lua_pushnumber(L,sqlite3_column_double(findOtherSide,1));
  lua_pushnumber(L,sqlite3_column_double(findOtherSide,2));
  sqlite3_reset(findOtherSide);
  return 3;
}

static int l_portal_disable(lua_State* L) {
  sqlite3_int64 id = lua_tonumber(L, 1);
  if(id < 0) {
    luaL_typerror(L, 1, "portal");
    return 0;
  }
  lua_pop(L,1);

  sqlite3_bind_null(setThisSide,1);
  sqlite3_bind_null(setThisSide,2);
  sqlite3_bind_null(setThisSide,3);
  sqlite3_bind_int64(setThisSide,4,id);
  int res = sqlite3_step(setThisSide);
  sqlite3_reset(setThisSide);
  lua_pushboolean(L,res == SQLITE_DONE);
  return 1;
}

static int l_portal_set_thisside(lua_State* L) {
  sqlite3_int64 id = lua_tonumber(L, 1);
  if(id <= 0) {
    luaL_typerror(L, 1, "portal ID");
    return 0;
  }

  if(lua_gettop(L) < 4) return 0;
  sqlite3_bind_double(setThisSide,1,lua_tonumber(L,2));
  sqlite3_bind_double(setThisSide,2,lua_tonumber(L,3));
  sqlite3_bind_double(setThisSide,3,lua_tonumber(L,4));
  lua_pop(L,4);
  sqlite3_bind_int64(setThisSide,4,id);
  int res = sqlite3_step(setThisSide);
  sqlite3_reset(setThisSide);
  lua_pushboolean(L,res==SQLITE_DONE);
  return 1;
}

static int l_portal_create(lua_State* L) {
  assert(database != NULL);
  int res = sqlite3_step(begin);
  if(res != SQLITE_DONE) {
    sqlite3_reset(begin);
    fprintf(stderr,"whuh??? %d %s\n",res,sqlite3_errmsg(database));
  }
  sqlite3_reset(begin);

  /* These shenanigans are OK 
     since BEGIN IMMEDIATE gets all the locks already. */
  res = sqlite3_step(maxPortal);
  if(res != SQLITE_ROW) {
    sqlite3_reset(maxPortal);
    sqlite3_step(rollback);
    sqlite3_reset(rollback);
    fprintf(stderr,"Max failed %d %s\n",res,sqlite3_errmsg(database));
    return 0;
  }
  sqlite3_int64 orange = sqlite3_column_int64(maxPortal,0) + 1;
  sqlite3_reset(maxPortal);
  sqlite3_int64 blue = orange + 1;

  fprintf(stderr,"Figured portals %lld %lld\n",orange,blue);

  sqlite3_bind_int64(insertPortal,1,orange);
  sqlite3_bind_int64(insertPortal,2,blue);  
  res = sqlite3_step(insertPortal);
  sqlite3_reset(insertPortal);
  if(res != SQLITE_DONE) {
    sqlite3_step(rollback);
    sqlite3_reset(rollback);
    fprintf(stderr,"Insert failed %d %s\n",res,sqlite3_errmsg(database));
    return 0;
  }

  sqlite3_bind_int64(insertPortal,1,blue);
  sqlite3_bind_int64(insertPortal,2,orange);  
  res = sqlite3_step(insertPortal);
  sqlite3_reset(insertPortal);
  if(res != SQLITE_DONE) {
    sqlite3_step(rollback);
    sqlite3_reset(rollback);
    fprintf(stderr,"Insert 2 failed %d %s\n",res,sqlite3_errmsg(database));
    return 0;
  }

  res = sqlite3_step(commit);
  sqlite3_reset(commit);
  if(res != SQLITE_DONE) {
    fprintf(stderr,"Commit failed %s\n",sqlite3_errmsg(database));
    return 0;
  }

  lua_pushinteger(L,orange);
  lua_pushinteger(L,blue);
  return 2;
}

static int fu(lua_State* L) {
  //lua_pop(L,1);
  lua_pushstring(L,"fuck you I'm an anteater");
  return 1;
}

void l_portals_register(lua_State* L) {
#define method(name) {#name, l_portal_ ## name}

  const luaL_reg methods[] = {
    method(setup),
    method(find_otherside),
    method(set_thisside),
    method(disable),
    method(create),
    {"derp",fu},
    {"__tostring", fu},
    {0,0}
  };

#undef method

  lua_newtable(L);
  luaL_register(L, NULL, methods);
  lua_setglobal(L, "portals");
  // Now you're playing with portals!
}
