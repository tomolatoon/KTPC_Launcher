# KTPC Launcher
駒場東邦物理部（KTPC）が文化祭の際にゲームを展示するために使用するゲームランチャーの 2023 年バージョンです。現在開発中です。

# How to use
完成したら書きます。

1. `data.schema.json` を実行ファイルと同じフォルダに置いて下さい。
2. `Mplus1-XXX.otf` を実行ファイルと同じフォルダに置いて下さい。

# How to build
OpenSiv3D v0.6.6 + 独自 OpenSiv3D v0.6.7（Experimental）を使用してビルドをします。

1. [OpenSiv3D v0.6.6](https://siv3d.github.io/ja-jp/download/windows/) をインストール
2. OpenSiv3D v0.6.7（Experimental）をビルド
   1. [tomolatoon/OpenSiv3D をクローンして json_validation ブランチに切り替える](https://github.com/tomolatoon/OpenSiv3D/tree/json_validation) 
   2. \[Windows\]: ビルドします
   3. \[macOS、Linux\]: `ThirdParty/nlohmann`以下のヘッダとソースファイルをビルドシステム（xcode、CMake）に追加してからビルドします
3. ビルドした OpenSiv3D v0.6.7（Experimental）をこのフォルダを基準として、`./ThirdParty/MySiv3D/include/`と`./ThirdParty/MySiv3D/lib/`にコピペします
4. [ICU](https://github.com/unicode-org/icu/releases) をインストール（[macOS, Linux の人はいい感じに ICU のガイドを見つつ読み替えて下さい](https://unicode-org.github.io/icu/userguide/icu/howtouseicu.html)）
   1. [ICU](https://github.com/unicode-org/icu/releases) からビルド済みバイナリ（icu4c-72_1-XXX-XXX.zip）をダウンロード
   2. 解凍（OS 標準機能でも可、7zip 等を使うと尚良し）
   3. `include`と名前が付いているディレクトリを`./ThirdParty/ICU/`にコピペ
   5. `lib`みたいな名前のディレクトリを`./ThirdParty/ICU/`にコピペ
   6. \[Windows\]: `bin`みたいな名前のディレクトリの中にある`.dll`を`C:/Windows/System32/`とかいい感じのところにコピペ
   7. \[macOS、Linux\]: 4. に対して、対応するファイル（`.dylib/.bundle/.so`辺り？）を、対応するいい感じにロードできそうなディレクトリに置きます
   4. \[macOS、Linux\]: `./ThirdParty/ICU/include`をインクルードディレクトリとしてビルドシステムに設定
   8. \[macOS、Linux\]: スタティックライブラリ（`.a`？）をリンクするようにビルドシステムに追加します
5. これをビルドします

## See also
- [Siv3D | ライブラリの自前ビルド](https://zenn.dev/reputeless/articles/article-build-siv3d)
- [Siv3D SDK を自前ビルドする手順｜Siv3D リファレンス v0.6.6](https://zenn.dev/reputeless/books/siv3d-documentation/viewer/build)
- [How To Use ICU | ICU Documentation](https://unicode-org.github.io/icu/userguide/icu/howtouseicu.html)