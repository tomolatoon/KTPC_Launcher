# KTPC Launcher
駒場東邦物理部（KTPC）が文化祭の際にゲームを展示するために使用するゲームランチャーの 2023 年バージョンです。現在開発中なので、コードは汚い（マジックナンバー・汎用性のなさ・純粋に読みづらい）ですが、一通り出来たらきれいにしていきます。

# How to use
完成したら書きます。

1. `data.schema.json` を実行ファイルと同じフォルダに置いて下さい。

# How to build
OpenSiv3D v0.6.7 を使用すればビルド出来るコードなので、2. を飛ばすことが可能です。
ただし、Visual Studio のソリューションをそのまま使用する場合は、自前ビルドが必要です。自前ビルドは次のように行ってください。

1. [OpenSiv3D v0.6.7](https://siv3d.github.io/ja-jp/) をインストール
2. OpenSiv3D v0.6.7 をビルド
   1. [Siv3D/OpenSiv3D](https://github.com/Siv3D/OpenSiv3D) を手元に用意（`git clone`や`GitHub Desktop`、Zip としてダウンロードなど）をする
   2. 各プラットフォーム用のからビルドをする（この際に、設定を変えてデバッグ用ファイルを吐き出させるのが吉、Visual Studio なら`/Zi`でよい）
3. ビルドした OpenSiv3D v0.6.7 をこのフォルダを基準として、`./ThirdParty/MySiv3D/include/`と`./ThirdParty/MySiv3D/lib/`にコピペします
4. [ICU](https://github.com/unicode-org/icu/releases) をインストール（[macOS, Linux の人はいい感じに ICU のガイドを見つつ読み替えて下さい](https://unicode-org.github.io/icu/userguide/icu/howtouseicu.html)）
   1. [ICU](https://github.com/unicode-org/icu/releases) からビルド済みバイナリ（icu4c-72_1-XXX-XXX.zip）をダウンロード
   2. 解凍（OS 標準機能でも可、7zip 等を使うと尚良し）
   3. `include`と名前が付いているディレクトリを`./ThirdParty/ICU/`にコピペ
   4. `lib`みたいな名前のディレクトリを`./ThirdParty/ICU/`にコピペ
   5. \[Windows\]: `bin`みたいな名前のディレクトリの中にある`.dll`を`C:/Windows/System32/`とかいい感じのところにコピペ
   6. \[macOS、Linux\]: 4. に対して、対応するファイル（`.dylib/.bundle/.so`辺り？）を、対応するいい感じにロードできそうなディレクトリに置きます
   7. \[macOS、Linux\]: `./ThirdParty/ICU/include`をインクルードディレクトリとしてビルドシステムに設定
   8. \[macOS、Linux\]: スタティックライブラリ（`.a`？）をリンクするようにビルドシステムに追加します
5. これをビルドします

## See also
- [Siv3D | ライブラリの自前ビルド](https://zenn.dev/reputeless/articles/article-build-siv3d)
- [Siv3D SDK を自前ビルドする手順｜Siv3D リファレンス v0.6.6](https://zenn.dev/reputeless/books/siv3d-documentation/viewer/build)
- [How To Use ICU | ICU Documentation](https://unicode-org.github.io/icu/userguide/icu/howtouseicu.html)