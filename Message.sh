xgettext \
	--package-name=fcitx-mcbopomofo \
	--package-version=v0.1 \
	--copyright-holder=OpenVanilla \
	-c+ \
	-k_ \
	-o po/fcitx-mcbopomofo.pot \
    src/McBopomofo.cpp src/McBopomofo.h

#msginit -l zh_TW.UTF-8 --no-translator -o po/zh_TW.po -i po/mcbopomofo.pot
