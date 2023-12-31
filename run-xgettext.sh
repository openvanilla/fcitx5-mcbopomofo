xgettext \
	--package-name=fcitx5-mcbopomofo \
	--package-version=2.4 \
	--copyright-holder=OpenVanilla \
	--c++ \
	--from-code=UTF-8 \
	-k_ \
	-kN_ \
	-o po/fcitx5-mcbopomofo.pot \
	src/McBopomofo.cpp src/McBopomofo.h src/DictionaryService.cpp

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
	src/mcbopomofo.conf.in.in src/mcbopomofo-plain.conf.in.in src/mcbopomofo-addon.conf.in.in

xgettext \
	--package-name=fcitx5-mcbopomofo \
	--copyright-holder=OpenVanilla \
	-j \
	-o po/fcitx5-mcbopomofo.pot \
	org.fcitx.Fcitx5.Addon.McBopomofo.metainfo.xml.in

## To re-generate the po files from scratch:
# msginit -l zh_TW.UTF-8 --no-translator -o po/zh_TW.po -i po/fcitx5-mcbopomofo.pot

msgmerge --update po/en.po po/fcitx5-mcbopomofo.pot
msgmerge --update po/zh_TW.po po/fcitx5-mcbopomofo.pot
