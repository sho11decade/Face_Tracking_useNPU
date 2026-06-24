# NPU_Vtube

Intel AI Boost (NPU) を使ったリアルタイム顔トラッキングを目指す C++20 プロジェクトです。

現時点では製品版アプリではなく、パイプライン骨格と Windows の Media Foundation カメラ入力を固める段階です。`tracker_dev` ではカメラ列挙、1 フレーム取得、モデル定義の解決、推論/追跡/マッピングのプレースホルダ経路確認ができます。

`TECHNICAL_DETAILS.md` は全体設計の詳細版です。本 README と `docs/` は、現在の実装状態を短く追える運用向けドキュメントとして整理しています。

## 現在の状態

実装済み:

- CMake ベースの C++20 プロジェクト構成
- `tracker_dev` 開発用アプリ
- Media Foundation によるカメラ列挙
- Media Foundation による単発フレーム取得
- 推論デバイス選択ロジックの最小実装
- モデルカタログとモデル解決ロジック
- OneEuro Filter の基盤
- 追跡状態更新とパラメータ変換の最小実装
- スモークテスト 1 本

未実装またはスタブ:

- OpenVINO による実モデル推論
- DirectX11 によるプレビュー/オーバーレイ描画
- Dear ImGui によるデバッグ UI
- 連続キャプチャを含む本格的なリアルタイムパイプライン

## リポジトリ構成

```text
.
|- apps/tracker_dev/        開発用エントリポイント
|- configs/default.json     既定設定のたたき台
|- docs/                    実装寄りドキュメント
|- include/npu_vtube/       公開ヘッダ
|- models/                  モデル配置場所
|- src/
|  |- core/                 アプリ情報などの基礎部品
|  |- capture/              Media Foundation キャプチャ
|  |- inference/            推論サービス骨格
|  |- tracking/             追跡サービス骨格
|  |- mapping/              パラメータ変換骨格
|  |- render/dx11/          描画バックエンド骨格
|  |- ui/imgui/             UI バックエンド骨格
|  `- platform/windows/     Windows 環境依存コード
|- tests/                   最小テスト
|- CMakeLists.txt           ルート CMake
|- vcpkg.json               依存マニフェスト
`- TECHNICAL_DETAILS.md     設計詳細
```

## ビルドとテスト

このワークスペースでは次のコマンドで確認しました。

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`tracker_dev` の起動例:

```powershell
.\build\apps\tracker_dev\tracker_dev.exe
```

補足:

- 現在の既定オプションでは `OpenVINO`、`OpenCV`、`Dear ImGui`、`oneTBB` はビルドに必須ではありません。
- `vcpkg.json` は将来の統合を見据えた依存宣言です。必要になった段階で `CMAKE_TOOLCHAIN_FILE` に vcpkg の toolchain を渡す運用を想定しています。
- 詳細は `docs/build.md` を参照してください。

## 現在の実装モジュール

`capture`:

- `CaptureService::list_devices()` で Media Foundation の映像入力デバイスを列挙
- `CaptureService::initialize()` でデバイスを開き、要求解像度に最も近いネイティブ形式を選択
- `CaptureService::grab_frame()` で 1 フレームを同期取得

`inference`:

- 利用可能デバイス一覧から `NPU -> GPU -> CPU` 優先で文字列選択
- `models/` 配下から推奨モデル定義を解決
- 観測値はまだ簡易推定

`tracking`:

- 顔矩形から中心座標とスケールを算出
- OneEuro Filter を使って中心/スケール/姿勢を平滑化

`mapping`:

- 追跡状態をそのまま `MappedParameters` へ移送する最小実装

`render` / `ui`:

- 状態保持の最小実装

## 次の実装マイルストーン

- `CaptureService` を単発取得から連続取得パイプラインへ拡張
- `configs/default.json` を実際に読み込む設定レイヤ追加
- OpenVINO 実推論の最小統合
- 追跡フィルタと正規化マッピングの具体化
- カメラプレビューとデバッグ表示の初期実装

## 参照ドキュメント

- `docs/architecture.md`: モジュール境界とデータフロー
- `docs/build.md`: 現在のビルド手順と CMake オプション
- `docs/capture.md`: Media Foundation キャプチャ実装の現状
- `docs/inference.md`: 推論層とモデル解決の現状
- `docs/models.md`: 顔検出・ランドマーク・頭部姿勢の推奨モデル
- `docs/roadmap.md`: 近接タスクと中長期目標
- `TECHNICAL_DETAILS.md`: 設計詳細と将来像
