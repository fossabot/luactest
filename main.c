
/**************************************************************************************************/
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/**************************************************************************************************/
#include "./lua/src/lauxlib.h"
#include "./lua/src/lua.h"
#include "./lua/src/lualib.h"
#include "./lua.c"
#include "./linenoise/linenoise.h"
#include "include.h"
#include "test.h"
/*********************************************CONFIGS**********************************************/
#define HISTORY_MAX_LEN 10000
#define H_FILE "./lct_history.lua"
#define LUA_RC "./defaults.lua"
/**************************************************************************************************/
lua_State* ls;

size_t get_str_len(const char* str) {
  int size_counter = 0;
  while (true) {
    if (str[size_counter] != '\0') {
      size_counter++;
    } else {
      return size_counter;
    }
  }
}

int devi_find_last_word(char const * const str, char* delimiters, int delimiter_size) {
  size_t str_len = get_str_len(str);
  bool is_delimiter = false;

  for (int i = 0; i < delimiter_size; ++i) {
    if (delimiters[i] == str[str_len-1]) return -1;
  }

  for (int i = str_len -1; i >=0; --i) {
    for (int j = 0; j < delimiter_size;++j) {
      if (delimiters[j] == str[i]) {
        return i + 1;
      } else {
        /*intentionally left blank*/
      }
    }
  }

  return 0;
}

size_t devi_rfind(char const * const str, char const * const substr) {
  size_t str_len = get_str_len(str);
  size_t substr_len = get_str_len(substr);
  for (size_t i = str_len-1; i >=0 ; --i) {
    if (substr[substr_len-1] != str[i]) {
      continue;
    } else {
      bool matched = true;
      for (int j = substr_len-1; j >= 0; --j) {
        if (substr[j] == str[i - (substr_len - j -1)]) {
          continue;
        } else {
          matched = false;
        }
      }
      if (matched) return (i - substr_len + 1);
    }
  }

  return -1;
}

size_t devi_lfind(char const * const str, char const * const substr) {
  size_t str_len = get_str_len(str);
  size_t substr_len = get_str_len(substr);
  for (size_t i = 0; i < str_len; ++i) {
    if (substr[0] != str[i]) {
      continue;
    } else {
      bool matched = true;
      for (size_t j = 0; j < substr_len; ++j) {
        if (i + j >= str_len) return -1;
        if (substr[j] == str[i+j]) {
          continue;
        } else {
          matched = false;
        }
      }
      if (matched) return (i);
    }
  }

  return -1;
}

void get_lua_global_table(lua_State* _ls) {
  lua_pushglobaltable(_ls);
  lua_pushnil(_ls);
  while(0 != lua_next(_ls, -2)) {
    printf("%s\t", lua_tostring(_ls, -2));
    printf(":\t%d\n", get_str_len(lua_tostring(_ls, -2)));
    lua_pop(_ls, 1);
  }
  lua_pop(_ls, 1);
}

void shell_completion(const char* buf, linenoiseCompletions* lc, size_t pos) {
  if (NULL != buf) {
    int last_word_index = devi_find_last_word(buf, ID_BREAKERS, NELEMS(ID_BREAKERS));
    char* last_word = malloc(strlen(buf) - last_word_index+1);
    last_word[strlen(buf) - last_word_index] = '\0';
    memcpy(last_word, &buf[last_word_index], strlen(buf) - last_word_index);
    char* ln_matched;
    for(int i=0; i < NELEMS(LUA_FUNCS); ++i) {
      if ( 0 == devi_lfind(LUA_FUNCS[i], last_word)) {
        ln_matched = malloc(last_word_index + strlen(LUA_FUNCS[i]) + 1);
        ln_matched[last_word_index + strlen(LUA_FUNCS[i])] = '\0';
        memcpy(ln_matched, buf, last_word_index);
        memcpy(&ln_matched[last_word_index], LUA_FUNCS[i], strlen(LUA_FUNCS[i]));
        linenoiseAddCompletion(lc, ln_matched, pos);
        free(ln_matched);
      }
    }

    lua_pushglobaltable(ls);
    lua_pushnil(ls);
    while(0 != lua_next(ls, -2)) {
      if ( 0 == devi_lfind(lua_tostring(ls, -2), last_word)) {
        ln_matched = malloc(last_word_index + strlen(lua_tostring(ls, -2)) + 1);
        ln_matched[last_word_index + strlen(lua_tostring(ls, -2))] = '\0';
        memcpy(ln_matched, buf, last_word_index);
        memcpy(&ln_matched[last_word_index], lua_tostring(ls, -2), strlen(lua_tostring(ls, -2)));
        linenoiseAddCompletion(lc, ln_matched, pos);
        free(ln_matched);
      }
      lua_pop(ls, 1);
    }
    lua_pop(ls, 1);

    free(last_word);
  }
}

char* shell_hint(const char* buf, int* color, int* bold) {
  printf(" %s ", buf);
  if (NULL != buf) {
      *color = 35;
      *bold = 0;
    }
  int last_word_index = devi_find_last_word(buf, ID_BREAKERS, NELEMS(ID_BREAKERS));
  char* last_word = malloc(strlen(buf) - last_word_index+1);
  last_word[strlen(buf) - last_word_index] = '\0';
  memcpy(last_word, &buf[last_word_index], strlen(buf) - last_word_index);
  char* ln_matched;
  for(int i=0; i < NELEMS(LUA_FUNCS); ++i) {
    size_t match_index = devi_lfind(LUA_FUNCS[i], last_word);
    if (0 == match_index) {
      ln_matched = malloc(strlen(LUA_FUNCS[i])-strlen(last_word)+1);
      ln_matched[strlen(LUA_FUNCS[i]) - strlen(last_word)] = '\0';
      memcpy(ln_matched, &LUA_FUNCS[i][match_index+strlen(last_word)], strlen(LUA_FUNCS[i]) - strlen(last_word));
      free(last_word);
      return ln_matched;
    }
  }

  lua_pushglobaltable(ls);
  lua_pushnil(ls);
  while(0 != lua_next(ls, -2)) {
    size_t match_index = devi_lfind(lua_tostring(ls, -2), last_word);
    if (0 == match_index) {
      ln_matched = malloc(strlen(lua_tostring(ls, -2))-strlen(last_word)+1);
      ln_matched[strlen(lua_tostring(ls, -2)) - strlen(last_word)] = '\0';
      memcpy(ln_matched, &lua_tostring(ls, -2)[match_index+strlen(last_word)], strlen(lua_tostring(ls, -2)) - strlen(last_word));
      free(last_word);
      return ln_matched;
    }
    lua_pop(ls, 1);
  }
  lua_pop(ls, 1);

  free(last_word);
  return NULL;
}

void lct_reg_all(lua_State * _ls)
{
  lua_register(_ls, "str2int", str2int_lct);
}
/**************************************************************************************************/
int main (int argc, char** argv) {
  char* command;
  linenoiseSetCompletionCallback(shell_completion);
  linenoiseSetHintsCallback(shell_hint);
  linenoiseHistorySetMaxLen(HISTORY_MAX_LEN);
  linenoiseHistoryLoad(H_FILE);
  linenoiseSetMultiLine(1);

  //int status, result;
  //lua_State *ls = luaL_newstate();
  ls = luaL_newstate();
  if (ls == NULL) {
    l_message(argv[0], "cannot create state: not enough memory");
    return EXIT_FAILURE;
  }

  luaL_openlibs(ls);
  lct_reg_all(ls);
  //get_lua_global_table(ls);
  dofile(ls, LUA_RC);
  // cli execution loop
  while (NULL != (command = linenoise("lct-0.1>>>"))) {
    linenoiseHistoryAdd(command);
    linenoiseHistorySave(H_FILE);
    dostring(ls, command, "lct_cli");
    linenoiseFree(command);
  }
#if 0
  lua_pushcfunction(ls, &pmain);
  lua_pushinteger(ls, argc);
  lua_pushlightuserdata(ls, argv);
  status = lua_pcall(ls, 2, 1, 0);
  result = lua_toboolean(ls, -1);
  report(ls, status);
  lua_close(ls);
  return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
}
/**************************************************************************************************/

