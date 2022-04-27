# fcitx5-mcbopomofo: 小麥注音輸入法 fcitx5 模組

![Build](https://github.com/openvanilla/fcitx5-mcbopomofo/actions/workflows/ci.yaml/badge.svg)

本專案是[小麥注音](https://github.com/openvanilla/McBopomofo)的 Linux 版本，功能與小麥注音 macOS 版 2.2 同步：

- 提供自動選字注音
- 用戶自訂詞功能：可在組字區加詞，亦可直接修改用戶詞庫檔案及刪詞設定檔
- 記憶使用者最近選字
- 支援標準、倚天、許氏鍵盤、倚天 26 鍵等常見注音鍵盤排列
- 便捷輸入各種中文標點符號
- 支援微軟新注音式「游標後」跟漢音式「游標前」選字

與 macOS 版的主要差別及功能限制如下：

- 注音組字目前只支援空白鍵，選字也只支援空白鍵
- 使用 fcitx5 內建選字窗
- 繁簡轉換使用 fcitx5 內建 chttrans 模組

此外，在專案組織上：

- 目前直接使用 McBopomofo macOS 版產生的資料檔
- 語言模型及選字引擎程式碼目前從 macOS 版拷貝過來

## 安裝方式

以下說明如何在 Ubuntu 21.10 以及 22.04 LTS 上面編譯安裝。

請先安裝 fcitx5, CMake, 以及以下開發用模組：

```
sudo apt install \
    fcitx5 libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev \
    cmake extra-cmake-modules gettext libfmt-dev
```

然後在本專案的 git 目錄下執行以下指令：

```
mkdir -p build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug
make
sudo make install

# 初次安裝後，執行以下指令，小麥注音 icon 就會出現在 fcitx5 選單中
sudo update-icon-caches /usr/share/icons/*
```

安裝後重新啟動 fcitx5，就會在設定中找到小麥注音。

### Ubuntu 20.04 LTS 上的編譯方式

Ubuntu 20.04 LTS 安裝的 fcitx5 版本老舊。如果要編譯，在使用 `cmake` 指令時，要加上 `-DUSE_LEGACY_FCITX5_API=1` 才能用舊式 API：

```
mkdir -p build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug -DUSE_LEGACY_FCITX5_API=1
make
```

## 在其他 Linux Distro 上安裝

* 我們計畫利用 [wiki](https://github.com/openvanilla/fcitx5-mcbopomofo/wiki#%E5%AE%89%E8%A3%9D%E8%AA%AA%E6%98%8E) 收集在其他 Linux distro 上安裝的方式，歡迎提供內容與指正。

## C++ 語法風格

本專案的程式碼大致採用 [Google C++ style](https://google.github.io/styleguide/cppguide.html)。我們已經設定好 `.clang-format` 檔案，請在發送 PR 前使用 clang-format 重整風格即可。

本專案跟 Google C++ style 不同的地方如下：

- 我們不使用 `snake_case` 變數名稱。變數或參數一律使用 `lowerCamelCase` 風格。
- 成員函數不使用 `Foo::PascalCaseMethod()`。成員函數一律使用 `Foo::lowerCamelCaseMethod()` 風格。

此外，`src/Engine/` 目錄裡的程式碼，傳統上使用 [WebKit style](https://webkit.org/code-style-guidelines/)，未來繼續使用 WebKit 風格。我們也在該目錄放置了符合該風格的 `.clang-format`。

我們也推薦使用 [cpplint](https://github.com/cpplint/cpplint) 檢查 C++ 常見問題。

## 社群公約

歡迎小麥注音 Linux 用戶回報問題與指教，也歡迎大家參與小麥注音開發。

我們採用了 GitHub 的[通用社群公約](https://github.com/openvanilla/fcitx5-mcbopomofo/blob/master/CODE_OF_CONDUCT.md)。公約的中文版請參考[這裡的翻譯](https://www.contributor-covenant.org/zh-tw/version/1/4/code-of-conduct/)。我們以上述公約，作為維護小麥注音社群的準則。
