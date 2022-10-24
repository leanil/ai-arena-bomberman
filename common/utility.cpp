#include "utility.h"

#include <iostream>
#include <stdexcept>

using namespace std;

void error(const string& message) {
#ifdef NDEBUG
  cerr << "ERROR " << message << endl;
#else
  throw runtime_error(message);
#endif
}
