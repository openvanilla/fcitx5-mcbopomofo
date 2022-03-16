# fcitx-mcbopomofo

README work in progress.

## Dev Setup

In addition to CMake, you'll need the following packages:

```
sudo apt install fcitx5 libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev
```

## Initial Config for McBopomofo

Put this in `~/.config/fcitx5/conf/mcbopomofo.conf`:

```
# Or set to eten
BopomofoKeyboardLayout=standard
```

TODO: Take advantage of fcitx5's config UI and enum types.

## Setting Up fcitx5 for Ubuntu 20 LTS.

You don't need this if you are on Ubuntu 21 or later.

Based on Ubuntu 20 LTS. By default there aren't GUI tools and after apt install
nothing is configured for you. Additional input methods won't be discovered by
fcitx5.

Also, do **NOT** install or use `fcitx`, which is the previous version. Making
the legacy fcitx your default input method manager would cause your GUI session
to crash, and you would not be able to log into your GUI session again until
you revert your changes in `.xinputrc`!

To fix that, create `~/.config/fcitx5/profile` after you have installed fcitx5:

```
[Groups/0]
Name=Default
Default Layout=us
DefaultIM=keyboard-us

[Groups/0/Items/0]
Name=keyboard-us
Layout=

[Groups/0/Items/1]
Name=mcbopomofo
Layout=

[GroupOrder]
0=Default
```

For Dvorak users, replace all `keyboard-us` instances to `keyboard-us-dvorak`,
and add `Layout=dvorak` in that US keyboard setting item.

Then, update your `~/.xinputrc`, change the `run_im` setting to:

```
run_im fcitx5
```

And update your `~/.xprofile`:

```
export GTK_IM_MODULE=fcitx5
export QT_IM_MODULE=fcitx5
export XMODIFIERS="@im=fcitx5"
```

After that, running `fcitx5` should give you an additional keyboard icon on the
top menu bar. You should be able to see McBopomofo listed among the available
input methods.
