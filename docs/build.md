# ビルド手順

## 前提

現時点のソースは CMake ベースの C++20 プロジェクトです。

必要条件:

- Windows 環境
- CMake 3.25 以上
- C++20 対応コンパイラ
- Media Foundation が利用できること

補足:

- ルートの `vcpkg.json` には `fmt`、`imgui`、`nlohmann-json`、`opencv`、`openvino`、`tbb` を宣言しています。
- ただし現在の既定ビルドでは `NPUVT_ENABLE_OPENVINO=OFF`、`NPUVT_ENABLE_OPENCV=OFF`、`NPUVT_ENABLE_IMGUI=OFF`、`NPUVT_ENABLE_TBB=OFF` のため、これらをまだ実リンクしていません。
- そのため、現状は「vcpkg マニフェストは用意済みだが、既定構成は軽量」という段階です。
- OpenVINO の推論コード自体は実装済みで、`NPUVT_ENABLE_OPENVINO=ON` のときに `find_package(OpenVINO REQUIRED COMPONENTS Runtime)` を通して有効化されます。

## このワークスペースで確認したコマンド

この環境では `MinGW Makefiles` と `g++ 15.2.0` で次を確認しました。

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

生成物:

- `build/apps/tracker_dev/tracker_dev.exe`
- `build/tests/npuvt_smoke_test.exe`

`tracker_dev` の起動:

```powershell
.\build\apps\tracker_dev\tracker_dev.exe
```

## vcpkg を使う場合の基本形

将来の依存統合を含める場合は、通常の CMake configure に vcpkg toolchain を渡します。

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

注意点:

- このリポジトリは現時点で vcpkg のライブラリを必須リンクしていないため、上記は将来の統合を見据えた標準形です。
- OpenVINO や ImGui を本当に使い始めた段階で、`find_package` とリンク設定の更新が必要です。

## CMake オプション

ルート `CMakeLists.txt` の主要オプション:

| オプション | 既定値 | 用途 |
| --- | --- | --- |
| `NPUVT_BUILD_APP` | `ON` | `tracker_dev` をビルド |
| `NPUVT_BUILD_TESTS` | `ON` | `tests/` をビルド |
| `NPUVT_ENABLE_OPENVINO` | `OFF` | OpenVINO 統合を有効化予定 |
| `NPUVT_ENABLE_OPENCV` | `OFF` | OpenCV 補助経路を有効化予定 |
| `NPUVT_ENABLE_IMGUI` | `OFF` | Dear ImGui 統合を有効化予定 |
| `NPUVT_ENABLE_TBB` | `OFF` | oneTBB 利用を有効化予定 |

既定値のままでも、現在のスケルトンと Media Foundation キャプチャはビルドできます。

OpenVINO 実行を有効にする例:

```powershell
cmake -S . -B build-openvino -DNPUVT_ENABLE_OPENVINO=ON -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build-openvino --config Debug
```

注意:

- この環境では OpenVINO パッケージ自体は未導入でした。
- そのため、`ON` 構成の実ビルドはまだ未確認です。

## テスト

現在の自動テストは `tests/smoke_test.cpp` の 1 本です。

確認内容:

- 推論デバイス選択の優先順位
- プレースホルダ観測値から `TrackingState` 生成
- `MappedParameters` への変換

まだ確認していないもの:

- 実カメラを使ったキャプチャの自動テスト
- OpenVINO 実推論
- 描画/UI の統合テスト

## 現状のビルド上の注意

- `src/capture` は Windows で Media Foundation ライブラリにリンクします。
- `tracker_dev` は現状コンソールアプリで、起動時にカメラ列挙と単発フレーム取得を試します。
- Visual Studio generator でも構成できる想定ですが、この作業では未検証です。
