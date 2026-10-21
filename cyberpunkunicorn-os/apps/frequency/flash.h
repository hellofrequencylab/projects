// The CYD's onboard RGB LED as the "it worked" signal: there is no speaker to beep.
#pragma once

namespace freq {

// Rainbow flashes, about 1.5 s, on its own task: returns at once, so a remote
// command answers without waiting for the light. A second call while one is
// running is ignored.
void flashSuccess();

} // namespace freq
