// src/custom_ota_override.cpp
// Strong override of the weak boot/OTA hook in the core.
// Returning true means: "do NOT auto-validate the running OTA image".
// After any reset, the bootloader will roll back to the last valid app (your factory).

extern "C" bool verifyRollbackLater();  // must use C linkage to match the weak symbol

extern "C" bool verifyRollbackLater() {
  return true;  // never auto-validate OTAs
}
