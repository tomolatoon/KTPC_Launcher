# KTPC Launcher
駒場東邦物理部（KTPC）が文化祭の際にゲームを展示するために使用するゲームランチャーの 2023 年バージョンです。現在開発中です。

# How to use
完成したら書きます。

# How to build
OpenSiv3D v0.6.6 + 独自 OpenSiv3D v0.6.7（Experimental）を使用してビルドをします。

1. [OpenSiv3D v0.6.6](https://siv3d.github.io/ja-jp/download/windows/) をインストール
2. OpenSiv3D v0.6.7（Experimental）をビルド
   1. [tomolatoon/OpenSiv3D をクローンして json_validation ブランチに切り替える](https://github.com/tomolatoon/OpenSiv3D/tree/json_validation) 
   2. \[Windows\]: ビルドします
   3. \[macOS、Linux\]: ThirdParty/nlohmann 以下のヘッダとソースファイルをプロジェクト（xcode、CMake）に追加してからビルドします
3. ビルドした OpenSiv3D v0.6.7（Experimental）をこのフォルダを基準として、`./MySiv3D/include/`と`./MySiv3D/lib/`にコピペします
4. これをビルドします

## See also
- [Siv3D | ライブラリの自前ビルド](https://zenn.dev/reputeless/articles/article-build-siv3d)
- [Siv3D SDK を自前ビルドする手順｜Siv3D リファレンス v0.6.6](https://zenn.dev/reputeless/books/siv3d-documentation/viewer/build)