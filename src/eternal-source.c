#include "sources.h"

gint main(gint argc, gchar* argv[]) {
  GList* sources;
  sources = get_eternal_sources();
  sources = get_sources_from_args(argc, argv, sources);
  write_sources(sources);
  g_list_free(sources);
  return 0;
}
