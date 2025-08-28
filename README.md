# fcitx5-mcbopomofo: 小麥注音輸入法 fcitx5 模組

![Build](https://github.com/openvanilla/fcitx5-mcbopomofo/actions/workflows/ci.yaml/badge.svg)

本專案是[小麥注音](https://github.com/openvanilla/McBopomofo)的 Linux 版本，功能與小麥注音 macOS 版同步：

- 提供自動選字注音
- 提供傳統注音（打一字選一字「ㄅ半」注音）
- 用戶自訂詞功能：可在組字區加詞，亦可直接修改用戶詞庫檔案及刪詞設定檔
- 記憶使用者最近選字
- 支援標準、倚天、許氏鍵盤、倚天 26 鍵等常見注音鍵盤排列
- 便捷輸入各種中文標點符號
- 支援微軟新注音式「游標後」跟漢音式「游標前」選字

與 macOS 版的主要差別如下：

- 使用 fcitx5 內建選字窗
- 繁簡轉換使用 fcitx5 內建 chttrans 模組

此外，在專案組織上：

- 目前直接使用 McBopomofo macOS 版產生的資料檔
- 語言模型及選字引擎程式碼目前從 macOS 版拷貝過來

## 安裝方式

以下說明如何在 Ubuntu 24.04 LTS（或是 22.04 LTS）上編譯安裝。

請先安裝 fcitx5, CMake, 以及以下開發用模組：

```bash
sudo apt install \
    pkg-config fcitx5 libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev fcitx5-modules-dev \
    cmake extra-cmake-modules gettext libfmt-dev libicu-dev libjson-c-dev
```

然後在本專案的 git 目錄下執行以下指令：

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build

# 初次安裝後，執行以下指令，小麥注音 icon 就會出現在 fcitx5 選單中
sudo update-icon-caches /usr/share/icons/*
```

## 在其他 Linux Distro 或 FreeBSD 上安裝

- 我們利用 [wiki](https://github.com/openvanilla/fcitx5-mcbopomofo/wiki#%E5%AE%89%E8%A3%9D%E8%AA%AA%E6%98%8E) 收集在其他 Linux distro 或 FreeBSD 上安裝的方式，歡迎提供內容與指正。

## C++ 語法風格

### clang-format

本專案的程式碼大致採用 [Google C++ style](https://google.github.io/styleguide/cppguide.html)。我們已經設定好 `.clang-format` 檔案，請在發送 PR 前使用 clang-format 重整風格即可。

```bash
find src/ -name *.cpp -o -name *.h | xargs clang-format -i
```

本專案跟 Google C++ style 不同的地方如下：

- 我們不使用 `snake_case` 變數名稱。變數或參數一律使用 `lowerCamelCase` 風格。
- 成員函數不使用 `Foo::PascalCaseMethod()`。成員函數一律使用 `Foo::lowerCamelCaseMethod()` 風格。

### cpplint

我們也推薦使用 [cpplint](https://github.com/cpplint/cpplint) 檢查 C++ 常見問題。

```bash
cpplint --filter=-build/c++11,-build/include_alpha,-build/include_order,-build/include_subdir,-readability/nolint,-runtime/references,-whitespace --recursive src/
```

### clang-tidy

編譯時開啟 `-DENABLE_CLANG_TIDY=On` cmake flag 以啟用 `clang-tidy` 檢查 (需要 clang 14+)。

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DENABLE_CLANG_TIDY=On
```

## 使用 Ninja 加快建置速度

如果系統已經安裝 [Ninja](https://ninja-build.org/)，可使用以下指令，讓 CMake 將 `make` 換成 `ninja`，加快建置速度。尤其在啟用 clang-tidy 檢查程式時，可[加速不少](https://github.com/openvanilla/fcitx5-mcbopomofo/issues/96)：

```
rm -rf build  # 如果先前已經使用 make，要先移除 build 目錄
cmake -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DENABLE_CLANG_TIDY=On
cmake --build build  # 使用 ninja 建置
```

## 社群公約

歡迎小麥注音 Linux 用戶回報問題與指教，也歡迎大家參與小麥注音開發。

我們採用了 GitHub 的[通用社群公約](https://github.com/openvanilla/fcitx5-mcbopomofo/blob/master/CODE_OF_CONDUCT.md)。公約的中文版請參考[這裡的翻譯](https://www.contributor-covenant.org/zh-tw/version/1/4/code-of-conduct/)。我們以上述公約，作為維護小麥注音社群的準則。
