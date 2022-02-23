#ifndef _FCITX5_MCBOPOMOFO_MCBOPOMOFO_H_
#define _FCITX5_MCBOPOMOFO_MCBOPOMOFO_H_

#include <fcitx/inputmethodengine.h>
#include <fcitx/addonfactory.h>

class McBopomofoEngine : public fcitx::InputMethodEngine {
    void keyEvent(const fcitx::InputMethodEntry & entry, fcitx::KeyEvent & keyEvent) override;
};

class McBopomofoEngineFactory : public fcitx::AddonFactory {
    fcitx::AddonInstance * create(fcitx::AddonManager * manager) override {
        FCITX_UNUSED(manager);
        return new McBopomofoEngine;
    }
};

#endif // _FCITX5_MCBOPOMOFO_MCBOPOMOFO_H_
