# fcitx5-mcbopomofo: 小麥注音輸入法 fcitx5 模組

## 安裝方式

目前僅測試過在 Ubuntu 21 上面編譯安裝。

請先安裝 fcitx5, CMake, 以及以下開發用模組：

```
sudo apt install fcitx5 libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev
```

然後在本專案的 git 目錄下執行以下指令：

```
mkdir -p build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug
make
sudo make install
```

之後就會在 fcitx5 設定中看到小麥注音。

## 專案狀態

本專案是小麥注音開發者為了能在 Linux 上使用而開發，目前是 MVP (minimally-viable product)，功能做到小麥注音 1.1 的程度。限制如下：

* 只實作最重要的按鍵，例如注音組字只支援空白鍵，選字也只支援空白鍵
* 只支援游標在詞後面的選字，不支援微軟式的游標在詞前面選字
* 選字窗用的是內建的 pageable candidate list, 事件處理也都從簡
* 沒有實作組字區有字時，按 Shift + 英文字母鍵的各種中英混打處理
* 沒有記憶用戶選字、用戶自訂詞等小麥注音 2.0 版之後才有的功能
