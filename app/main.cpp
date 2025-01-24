#include "run.h"

int
main()
{
  extension_registry ext_registry;
  return run_landbrush_app(ext_registry, "landbrush") ? EXIT_SUCCESS : EXIT_FAILURE;
}
