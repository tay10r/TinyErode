#include "run.h"

int main()
{
  return run_landbrush_app(extension_registry(), "landbrush") ? EXIT_SUCCESS : EXIT_FAILURE;
}
