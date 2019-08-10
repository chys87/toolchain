#include "utils.h"
#include <stdint.h>

int strnumcmp(const char *a, const char *b) noexcept {
  const uint8_t *u = (const uint8_t *)a;
  const uint8_t *v = (const uint8_t *)b;
  unsigned U, V;

  do {
    U = *u++;
    V = *v++;
  } while ((U == V) && V);
  if (U == V)
    return 0;

  int cmpresult = U - V;

  // Both are digits - Compare as numbers:
  // We use the following rule:
  // If the two series of digits are of unequal length,
  // the longer one is larger;
  // otherwise, the comparison is straightforward
  for (;;) {
    U -= '0';
    V -= '0';
    if (U < 10) {
      if (V >= 10) {
        // Only U is a digit
        return 1;
      }
    } else if (V < 10) {
      // Only V is a digit
      return -1;
    } else {
      // Neither is a digit
      return cmpresult;
    }
    U = *u++;
    V = *v++;
  }
}
