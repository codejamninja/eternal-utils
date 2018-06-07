#include <pcre.h>
#include <glib.h>
#include "shared.h"

gchar* encode_key(GHashTable* envs, gchar* key) {
  if (g_hash_table_lookup(envs, key) != NULL) {
    GList* matches;
    gint count;
    count = 0;
    matches = regex("([0-9a-zA-Z_]+)(-[0-9]+)?", key, 0);
    if (g_list_length(matches) >= 2) {
      gchar* body;
      gchar* tail;
      if (g_list_length(matches) >= 3) {
        body = g_list_nth(matches, 1)->data;
        tail = g_list_nth(matches, 2)->data;
        if (strlen(tail) > 0) {
          count = atoi(tail + 1) + 1;
        }
      } else {
        body = key;
      }
      key = g_strconcat(body, "-", g_strdup_printf("%i", count), NULL);
      key = encode_key(envs, key);
    }
    g_list_free(matches);
  }
  return key;
}

gchar* decode_key(gchar* key) {
  gchar* found;
  found = strchr(key, '-');
  if (found != NULL) {
    g_utf8_strncpy(key, key, strlen(key)-strlen(found));
  }
  return key;
}

GHashTable* get_envs_from_content(char* content) {
  GHashTable* envs;
  char* line;
  envs = g_hash_table_new(g_str_hash, g_str_equal);
  line = strtok(content, "\n");
  while(line) {
    GList* matches;
    matches = regex("([ \t]*#[ \t]*)?(export[ \t]+)?([0-9a-zA-Z_]+)[ \t]*=[ \t]*([^\n]*)",
                    line, PCRE_MULTILINE||PCRE_EXTENDED);
    if (g_list_length(matches) >= 5) {
      if (strlen(g_list_nth(matches, 1)->data) <= 0) {
        gchar* key;
        gchar* value;
        key = g_list_nth(matches, 3)->data;
        key = encode_key(envs, key);
        value = trim(g_list_nth(matches, 4)->data);
        g_hash_table_insert(envs, key, value);
      }
    }
    g_list_free(matches);
    line = strtok(NULL, "\n");
  }
  return envs;
}

gchar* get_content_from_envs(GHashTable* envs) {
  gchar* content;
  content = "#!/bin/bash\n\n";
  GList* keys = g_hash_table_get_keys(envs);
  GList* l;
  for (l = keys; l != NULL; l = l->next) {
    gchar* key;
    gchar* value;
    key = decode_key(l->data);
    value = g_hash_table_lookup(envs, key);
    content = g_strconcat(content, "export ", key, "=\"", value, "\"\n", NULL);
  }
  g_list_free(l);
  g_list_free(keys);
  return content;
}

gchar* get_envs_path() {
  gchar* envs_filename;
  gchar* envs_path;
  gchar* shell;
  shell = get_shell();
  envs_filename = g_strconcat(".", shell, "_envs", NULL);
  envs_path = g_build_path(G_DIR_SEPARATOR_S, g_get_home_dir(), envs_filename, NULL);
  return envs_path;
}

GHashTable* get_envs_from_args(gint argc, gchar* argv[], GHashTable* envs) {
  gchar* command;
  command = "export";
  for (int i = 1; i < argc; i++) {
    const gchar delim[2] = "=";
    gchar* key;
    gchar* token;
    gchar* value;
    gchar* arg = g_malloc(strlen(argv[i])+1);
    strcpy(arg, argv[i]);
    token = strtok(arg, delim);
    value = "";
    for(int i = 0; i < 2; i++) {
      if (token != NULL) {
        if (i == 0) {
          key = token;
        } else if (i == 1) {
          value = token;
        } else {
          break;
        }
        token = strtok(NULL, delim);
      } else {
        break;
      }
    }
    g_free(arg);
    if (strlen(key) > 0) {
      g_hash_table_insert(envs, key, value);
      command = g_strconcat(command, " ", key, " \"", value, "\"", NULL);
    }
  }
  int err = system(command);
  if (err) {
    exit(1);
  }
  return envs;
}

GHashTable* get_eternal_envs(gint argc, gchar* argv[]) {
  GHashTable* envs;
  gchar* content;
  gchar* envs_path;
  envs_path = get_envs_path();
  content = read_file(envs_path);
  envs = get_envs_from_content(content);
  envs = get_envs_from_args(argc, argv, envs);
  g_free(envs_path);
  g_free(content);
  return envs;
}