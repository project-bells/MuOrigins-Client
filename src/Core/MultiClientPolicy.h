#pragma once

// Reserves one of the configured per-installation client slots. The handle is
// intentionally held for the process lifetime, so Windows frees the slot even
// after a crash. Returns false when every slot is already occupied.
bool MultiClientPolicy_AcquireSlot();
