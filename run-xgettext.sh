xgettext \
	--package-name=fcitx-mcbopomofo \
	--package-version=v0.1 \
	--copyright-holder=OpenVanilla \
	--c++ \
	-k_ \
	-kN_ \
	-o po/fcitx-mcbopomofo.pot \
    src/McBopomofo.cpp src/McBopomofo.h src/KeyHandler.cpp src/KeyHandler.h

## To re-generate the po files from scratch:
# msginit -l zh_TW.UTF-8 --no-translator -o po/zh_TW.po -i po/po/fcitx-mcbopomofo.pot

msgmerge --update po/en.po po/fcitx-mcbopomofo.pot
msgmerge --update po/zh_TW.po po/fcitx-mcbopomofo.pot
