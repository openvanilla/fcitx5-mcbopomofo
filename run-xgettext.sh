xgettext \
	--package-name=fcitx5-mcbopomofo \
	--package-version=v0.1 \
	--copyright-holder=OpenVanilla \
	--c++ \
	-k_ \
	-kN_ \
	-o po/fcitx5-mcbopomofo.pot \
    src/McBopomofo.cpp src/McBopomofo.h src/KeyHandler.cpp src/KeyHandler.h

xgettext \
	--package-name=fcitx5-mcbopomofo \
	--language=Desktop \
	-k \
	--keyword=Name \
	--keyword=GenericName \
	--keyword=Comment \
	--keyword=Keywords \
	-j \
	-o po/fcitx5-mcbopomofo.pot \
	src/mcbopomofo.conf.in.in src/mcbopomofo-addon.conf.in.in


## To re-generate the po files from scratch:
# msginit -l zh_TW.UTF-8 --no-translator -o po/zh_TW.po -i po/po/fcitx5-mcbopomofo.pot

msgmerge --update po/en.po po/fcitx5-mcbopomofo.pot
msgmerge --update po/zh_TW.po po/fcitx5-mcbopomofo.pot
