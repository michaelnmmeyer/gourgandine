#include <lua.h>
#include <lauxlib.h>
#include "../gourgandine.h"
#include "../src/lib/mascara.h"

#define GN_MT "gourgandine"

static int gn_lua_new(lua_State *lua)
{
   struct gourgandine **gn = lua_newuserdata(lua, sizeof *gn);
   *gn = gn_alloc();
   luaL_getmetatable(lua, GN_MT);
   lua_setmetatable(lua, -2);
   return 1;
}

static int gn_lua_fini(lua_State *lua)
{
   struct gourgandine **gn = luaL_checkudata(lua, 1, GN_MT);
   gn_dealloc(*gn);
   return 0;
}

static int gn_lua_extract(lua_State *lua)
{
   struct gourgandine **gn = luaL_checkudata(lua, 1, GN_MT);
   size_t len;
   const char *str = luaL_checklstring(lua, 2, &len);
   const char *lang = luaL_optstring(lua, 3, "en");

   struct mascara *mr = mr_alloc(lang, MR_SENTENCE);
   if (!mr)
      return luaL_error(lua, "no tokenizer for '%s'", lang);

   mr_set_text(mr, str, len);
   struct mr_token *sent;
   size_t sent_len = mr_next(mr, &sent);

   lua_newtable(lua);
   if (sent_len) {
      struct gn_acronym def = {0};
      size_t i = 0;
      while (gn_search(*gn, sent, sent_len, &def)) {
         lua_pushlstring(lua, def.acronym, def.acronym_len);
         lua_rawseti(lua, -2, ++i);
         lua_pushlstring(lua, def.expansion, def.expansion_len);
         lua_rawseti(lua, -2, ++i);
      }
   }
   mr_dealloc(mr);
   return 1;
}

int luaopen_gourgandine(lua_State *lua)
{
   const luaL_Reg abbr_rec_methods[] = {
      {"__gc", gn_lua_fini},
      {"extract", gn_lua_extract},
   };
   luaL_newmetatable(lua, GN_MT);
   lua_pushvalue(lua, -1);
   lua_setfield(lua, -2, "__index");
   luaL_setfuncs(lua, abbr_rec_methods, 0);

   const luaL_Reg abbr_lib[] = {
      {"new", gn_lua_new},
      {NULL, NULL},
   };
   luaL_newlib(lua, abbr_lib);
   return 1;
}
