#include "hw/border.h"

extern "C" void hwBorderAlertSetInternal(bool on);

void hwBorderAlert(bool on) { hwBorderAlertSetInternal(on); }
