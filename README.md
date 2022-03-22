# fcitx5-mcbopomofo: 小麥注音輸入法 fcitx5 模組

![Build](https://github.com/openvanilla/fcitx5-mcbopomofo/actions/workflows/ci.yaml/badge.svg)

本專案是[小麥注音](https://github.com/openvanilla/McBopomofo)開發者為了能在 Linux 上使用而開發，目前是 MVP (minimally-viable product)，功能做到小麥注音 1.1 的程度。限制如下：

- 只實作最重要的按鍵，例如注音組字只支援空白鍵，選字也只支援空白鍵
- 選字窗用的是內建的 pageable candidate list, 事件處理一切從簡
- 沒有實作組字區有字時，按 Shift + 英文字母鍵的各種中英混打處理
- 沒有記憶用戶選字、用戶自訂詞等小麥注音 2.0 版之後才有的功能

此外，在專案組織上：

- 目前直接使用 McBopomofo macOS 版產生的資料檔
- 語言模型及選字引擎程式碼目前從 macOS 版拷貝過來

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


