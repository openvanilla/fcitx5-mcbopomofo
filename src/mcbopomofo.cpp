#include "mcbopomofo.h"

void McBopomofoEngine::keyEvent(const fcitx::InputMethodEntry& entry, fcitx::KeyEvent& keyEvent)
{
    FCITX_UNUSED(entry);
    FCITX_INFO() << keyEvent.key() << " isRelease=" << keyEvent.isRelease();
}

FCITX_ADDON_FACTORY(McBopomofoEngineFactory);
