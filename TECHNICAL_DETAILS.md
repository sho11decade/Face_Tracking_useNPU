# Intel AI Boost (NPU) を活用したリアルタイム顔トラッキングソフト 技術詳細

## 1. 目的

本ドキュメントは、Intel AI Boost を搭載した NPU 対応 PC 上で動作する、リアルタイム顔トラッキングソフトの技術設計を整理するための設計書である。

初期版の主目的は以下とする。

- Web カメラ映像から顔を検出する
- 顔ランドマークおよび頭部姿勢を推定する
- 時系列フィルタでジッタを抑制する
- VTuber 用パラメータへ変換する
- リアルタイムに可視化する

初期スコープでは、表情の高自由度推定よりも、低遅延かつ安定した `顔位置`、`顔スケール`、`head yaw/pitch/roll` の取得を優先する。

## 2. 前提条件

### 2.1 採用技術

| 項目 | 採用方針 |
| --- | --- |
| 言語 | C++20 |
| AI Runtime | OpenVINO Runtime |
| モデル形式 | ONNX を正本とし、必要に応じて OpenVINO IR へ変換 |
| 学習 | PyTorch |
| カメラ | Media Foundation を主経路、OpenCV を補助経路 |
| フィルタ | OneEuro Filter |
| 描画 | DirectX11 |
| GUI | Dear ImGui |
| 並列処理 | std::jthread / std::thread + oneTBB |
| ビルド | CMake |
| パッケージ管理 | vcpkg |

### 2.2 初期版の対象機能

- 単一人物の顔追跡
- 顔矩形の推定
- 顔中心座標の追跡
- 顔スケールの推定
- 頭部姿勢角の推定
- デバッグ可視化
- デバイス選択とフォールバック

初期版では、以下は後続フェーズとする。

- ARKit 52 相当のブレンドシェイプ推定
- 複数人同時追跡
- Qt ベース製品 UI
- 配信ソフト連携機能

## 3. システム全体アーキテクチャ

全体パイプラインは次の通りである。

```text
Camera
  -> Capture Thread
  -> Inference Thread
  -> Tracking Filter
  -> Parameter Mapping
  -> Renderer
  -> UI
```

より具体的には、次の段階に分解する。

```text
[Camera / Media Foundation]
        |
        v
[Capture Thread]
  - frame acquisition
  - timestamping
  - optional color conversion
        |
        v
[Inference Thread]
  - preprocess
  - face detection
  - landmark / head pose inference
  - NPU / GPU / CPU execution
        |
        v
[Tracking Filter]
  - OneEuro Filter
  - outlier rejection
        |
        v
[Parameter Mapping]
  - screen-space to normalized values
  - head pose normalization
        |
        v
[Renderer / DX11]
  - camera preview
  - overlay
  - debug visuals
        |
        v
[Dear ImGui UI]
  - device selection
  - performance metrics
  - filter tuning
```

## 4. 設計方針

### 4.1 最重要目標

- 低遅延
- 安定追跡
- デバッグ容易性
- デバイス差異への耐性
- 将来の機能追加余地

### 4.2 初期版で優先しないもの

- 過度な抽象化
- 複雑なジョブシステム
- 多数の AI モデル同時稼働
- 高度なレンダリング演出

初期版では、「動く」「計測できる」「ボトルネックが見える」ことを優先する。

## 5. OpenVINO / NPU 前提の設計制約

Intel NPU を実運用に使う際、モデル選定とパイプライン設計には制約がある。

### 5.1 モデル形状

- NPU は `static shape` 前提で設計する
- 入力解像度は固定値とする
- 初期版のバッチサイズは `1` 固定とする
- 動的 shape 前提モデルは避ける

推奨例:

- 顔検出入力: `1 x 3 x 320 x 320`
- ランドマーク入力: `1 x 3 x 192 x 192`

### 5.2 精度と推論設定

- 実行方針は `latency-first` とする
- OpenVINO では `PERFORMANCE_HINT=LATENCY` を基本とする
- モデルは FP16 を基準に検証する
- 量子化モデルを使う場合も、NPU 上での実コンパイル確認を必須とする

### 5.3 デバイス戦略

リアルタイムアプリでは、予測可能な遅延が重要である。そのため、初期版は `明示デバイス選択` を基本とする。

優先順位は次とする。

1. `NPU`
2. `GPU`
3. `CPU`

運用上の推奨:

- 通常動作では明示的に `NPU` を指定する
- NPU でモデルコンパイルに失敗した場合は `GPU` へ退避する
- GPU も利用できない場合は `CPU` へ退避する
- 開発モードでは `AUTO:NPU,GPU,CPU` を許可してもよい

### 5.4 HETERO の扱い

- `HETERO` を初期版の主戦略にはしない
- モデル分割による実行先の不透明化を避ける
- デバッグ性と遅延予測性を優先する

### 5.5 キャッシュ

- `ov::cache_dir` を設定し、初回コンパイル後の起動時間を短縮する
- NPU ドライバや OpenVINO バージョン差異を考慮し、事前コンパイル済み blob の配布は前提にしない

### 5.6 Windows 依存事項

- Intel NPU driver の存在確認を起動時に行う
- Windows 11 環境を主対象とする
- 利用可能デバイス一覧を UI に表示する

## 6. モデル構成

### 6.1 初期版の推奨構成

初期版では、1 段の巨大モデルではなく、2 段構成を推奨する。

1. 顔検出モデル
2. 顔ランドマーク / 頭部姿勢推定モデル

この構成の利点は以下である。

- デバッグしやすい
- モデル差し替えが容易
- NPU 適合性の評価を段階的に行える
- 将来的な複数人追跡にも拡張しやすい

### 6.2 顔検出モデル要件

- 小型で低遅延
- 単一顔の安定検出が可能
- NPU でコンパイル可能
- 入力サイズ固定

出力例:

- `bbox x`
- `bbox y`
- `bbox width`
- `bbox height`
- `confidence`

### 6.3 頭部姿勢 / ランドマークモデル要件

- 顔 crop を入力とする
- 主要ランドマーク点を出力できる
- yaw / pitch / roll を直接または間接的に取得できる
- 小さな顔回転に対して安定である

出力例:

- `landmarks[0..N-1]`
- `yaw`
- `pitch`
- `roll`

## 7. スレッド構成

### 7.1 基本構成

初期版では、以下の 4 スレッド構成を基本とする。

1. `Capture Thread`
2. `Inference Thread`
3. `Filter Thread`
4. `Main/UI Thread`

### 7.2 各スレッドの責務

#### Capture Thread

- カメラ初期化
- フレーム取得
- タイムスタンプ付与
- 必要最小限の前処理
- 最新フレームの投入

#### Inference Thread

- 入力フレームの受信
- リサイズ、色空間変換
- 顔検出
- 顔 crop 生成
- ランドマーク / 頭部姿勢推論
- 観測結果の出力

#### Filter Thread

- OneEuro Filter 適用
- 外れ値除去
- 観測途切れ時の処理
- 正規化パラメータへの変換

#### Main/UI Thread

- DirectX11 初期化
- Dear ImGui 更新
- テクスチャ更新
- オーバーレイ描画
- デバッグ情報表示
- 終了制御

### 7.3 キュー戦略

リアルタイム性を優先し、`最新優先` の有界キューを採用する。

- `Capture -> Inference`: 容量 1 または 2
- `Inference -> Filter`: 容量 1 または 2
- `Filter -> UI`: 容量 1

キューが満杯の場合は、古いデータを捨てて最新を残す。

この方針により、処理遅延の蓄積を防ぎ、体感上の追従性を維持しやすくなる。

### 7.4 停止制御

- `std::jthread` と `stop_token` を優先採用する
- 終了時は capture から順に停止要求を伝播する
- UI 終了時に全スレッドを安全停止する

## 8. データ構造

### 8.1 FramePacket

```cpp
struct FramePacket {
    uint64_t frame_id;
    std::chrono::steady_clock::time_point timestamp;
    int width;
    int height;
    PixelFormat format;
    ImageBuffer image;
};
```

### 8.2 FaceObservation

```cpp
struct FaceObservation {
    uint64_t frame_id;
    std::chrono::steady_clock::time_point timestamp;
    bool detected;
    Rect2f face_bbox;
    std::vector<Vec2f> landmarks;
    float yaw;
    float pitch;
    float roll;
    float confidence;
};
```

### 8.3 TrackingState

```cpp
struct TrackingState {
    uint64_t frame_id;
    std::chrono::steady_clock::time_point timestamp;
    bool valid;
    Vec2f face_center;
    float face_scale;
    float yaw;
    float pitch;
    float roll;
};
```

### 8.4 MappedParameters

```cpp
struct MappedParameters {
    float head_x;
    float head_y;
    float head_z;
    float angle_yaw;
    float angle_pitch;
    float angle_roll;
};
```

### 8.5 PerfSnapshot

```cpp
struct PerfSnapshot {
    float capture_ms;
    float preprocess_ms;
    float inference_ms;
    float filter_ms;
    float render_ms;
    float end_to_end_ms;
    float fps;
    std::string device_name;
};
```

## 9. カメラ入力設計

### 9.1 Media Foundation を主経路にする理由

- Windows ネイティブで安定しやすい
- デバイス列挙やフォーマット制御がしやすい
- 将来的な低コピー化に有利

### 9.2 OpenCV の役割

- 早期開発の簡易入力
- 動画ファイル再生
- フォールバック入力

### 9.3 取り扱う画素形式

初期版では実装の単純さを優先する。

- 入力は `BGR` または `RGB` に正規化する
- 最適化段階で `NV12` 直処理を検討する

### 9.4 解像度方針

- 初期版は `1280x720` を推奨
- 推論入力は検出用に縮小
- 表示用は元解像度または縮小版を利用

## 10. 推論パイプライン設計

### 10.1 推論ステップ

1. フレーム受信
2. 顔検出用入力へ変換
3. 顔検出推論
4. 最良候補の顔を選択
5. 顔 crop を作成
6. ランドマーク / 頭部姿勢推論
7. 観測結果を構築

### 10.2 顔選択戦略

初期版では単一顔を対象にするため、次の基準で 1 件を選ぶ。

- 最高 confidence
- 直前フレーム位置との近さ
- 画面中央への近さ

この 3 指標のスコアリングで最終的な追跡対象を決定する。

### 10.3 前処理の責務

- リサイズ
- 色変換
- 正規化
- CHW 変換

前処理は OpenVINO の PrePostProcessor に寄せられる部分は寄せ、CPU 側処理を減らす。

## 11. フィルタリング設計

### 11.1 OneEuro Filter 採用理由

- 実装が比較的単純
- 低遅延用途に向く
- 動きが速い時は追従し、静止時は滑らかにしやすい

### 11.2 適用対象

初期版でフィルタをかける対象は以下とする。

- `face_center.x`
- `face_center.y`
- `face_scale`
- `yaw`
- `pitch`
- `roll`

### 11.3 外れ値処理

以下のケースでは観測値をそのまま採用しない。

- 顔矩形が急激に飛ぶ
- 角度が物理的に不自然に跳ぶ
- confidence が閾値未満

必要に応じて次を行う。

- 前回値保持
- 緩やかな補間
- `valid=false` として上位に通知

## 12. パラメータマッピング

### 12.1 初期版の出力パラメータ

- `head_x`
- `head_y`
- `head_z`
- `angle_yaw`
- `angle_pitch`
- `angle_roll`

### 12.2 正規化方針

- 画面中心を基準原点とする
- 顔中心の偏差を `[-1.0, 1.0]` に正規化する
- 顔サイズを `head_z` 相当の指標に変換する
- 角度は必要に応じてアプリ側定義レンジへクリップする

### 12.3 キャリブレーション

将来対応として、以下のキャリブレーションを設ける余地を残す。

- ニュートラル顔位置の学習
- 個人差の角度オフセット補正
- カメラ位置依存のスケール補正

## 13. 描画設計

### 13.1 DirectX11 採用理由

- Windows との親和性が高い
- Dear ImGui の実装例が豊富
- 初期のデバッグ描画が行いやすい

### 13.2 初期版の描画要素

- カメラプレビュー
- 顔矩形
- ランドマーク点
- 頭部姿勢軸
- 状態テキスト
- パフォーマンスメトリクス

### 13.3 レンダリングループ

毎フレームの大まかな処理順は次とする。

1. 最新 `TrackingState` を取得
2. カメラフレームをテクスチャへ転送
3. オーバーレイ描画
4. Dear ImGui ウィンドウ描画
5. Present

## 14. GUI 設計

### 14.1 Dear ImGui の役割

- 開発中の可視化
- 実行パラメータ調整
- デバイス切り替え
- フィルタ係数変更
- ログ確認

### 14.2 初期 UI 項目

- 使用デバイス表示
- 推論モデル表示
- FPS
- 各処理段の処理時間
- 顔検出 confidence
- フィルタ設定
- デバッグ描画 ON/OFF

### 14.3 Qt への移行を考慮した分離

UI 実装に依存しないよう、以下を分離する。

- `core`: ドメインロジック
- `ui`: パラメータ編集と表示
- `render`: 描画 backend

## 15. 並列処理設計

### 15.1 基本方針

- スレッド管理は標準ライブラリ中心
- データ並列が必要な箇所のみ oneTBB を使う

### 15.2 oneTBB の適用候補

- 画素変換
- 前処理バッファ生成
- 将来の複数顔後処理
- ベンチマーク用途の並列試験

### 15.3 避けるべきこと

- 初期版から全面的にタスクグラフ化すること
- スレッド境界とジョブ境界を過度に増やすこと
- UI スレッドと推論スレッドを密結合にすること

## 16. エラーハンドリング

### 16.1 起動時チェック

- カメラ接続有無
- モデルファイル存在
- OpenVINO Runtime 初期化可否
- 利用可能デバイス列挙可否
- NPU ドライバ有無

### 16.2 実行時エラー

- カメラフレーム欠落
- モデル推論失敗
- デバイスコンパイル失敗
- UI 初期化失敗

エラー時方針:

- 可能な限り graceful degradation を行う
- NPU 失敗時は GPU / CPU へ退避する
- UI に状態表示する
- 例外は最上位で捕捉し、再起動可能な構造にする

## 17. ログと計測

### 17.1 必須計測項目

- capture 時間
- preprocess 時間
- inference 時間
- filter 時間
- render 時間
- end-to-end 遅延
- current FPS

### 17.2 計測方針

- `std::chrono::steady_clock` を利用する
- フレーム単位で `frame_id` を採番する
- ログと UI 表示の両方に同一情報を流す

### 17.3 目的

- NPU 利用時の実遅延把握
- CPU ボトルネック発見
- カメラから描画までの遅延分析
- 最適化前後の比較

## 18. 推奨ディレクトリ構成

```text
.
|-- CMakeLists.txt
|-- vcpkg.json
|-- cmake/
|-- apps/
|   `-- tracker_dev/
|-- include/
|   `-- npu_vtube/
|-- src/
|   |-- core/
|   |-- capture/
|   |-- inference/
|   |-- tracking/
|   |-- mapping/
|   |-- render/
|   |   `-- dx11/
|   |-- ui/
|   |   `-- imgui/
|   `-- platform/
|       `-- windows/
|-- models/
|-- configs/
|-- assets/
`-- tests/
```

## 19. CMake ターゲット構成案

推奨ターゲット分割は以下である。

- `npuvt_core`
- `npuvt_capture`
- `npuvt_inference_ov`
- `npuvt_tracking`
- `npuvt_mapping`
- `npuvt_render_dx11`
- `npuvt_ui_imgui`
- `npuvt_app_dev`

この分割により、以下がしやすくなる。

- 単体テスト
- 将来の Qt UI 差し替え
- OpenGL backend 差し替え
- 推論モジュール単独ベンチマーク

## 20. vcpkg 依存関係案

初期版の候補依存は以下。

- `openvino`
- `opencv`
- `imgui`
- `onetbb`
- `nlohmann-json`
- `fmt` または `spdlog`
- `gtest` または `doctest`

補助候補:

- `eigen3`

依存は最小限から始め、必要時に追加する。

## 21. テスト戦略

### 21.1 単体テスト対象

- OneEuro Filter
- パラメータマッピング
- 顔選択ロジック
- 外れ値除去ロジック

### 21.2 結合テスト対象

- カメラ入力からレンダラまでの疎通
- OpenVINO デバイス切り替え
- モデル読込と推論
- UI パラメータ変更反映

### 21.3 リプレイテスト

- 録画動画を入力にした再現可能テスト
- 同一データで最適化前後の比較
- フィルタ係数変更時の挙動比較

## 22. 性能目標

初期版の目標値は以下とする。

- 入力解像度: `1280x720`
- 動作フレームレート: `30 FPS` 以上
- 可能なら `60 FPS` を目標
- end-to-end 遅延: `50 ms` 前後以下
- フレーム詰まり時でも最新フレーム優先を維持

## 23. 主な技術リスク

### 23.1 NPU 非対応演算

対策:

- モデル候補を早期に NPU でコンパイル検証する
- 軽量モデルを優先する

### 23.2 カメラ処理のコピー過多

対策:

- バッファ所有権を明確化する
- 必要最小限の色変換に留める

### 23.3 フィルタでの遅延増加

対策:

- OneEuro Filter のパラメータを UI から調整可能にする
- 生値とフィルタ後値を両方表示する

### 23.4 UI と処理系の干渉

対策:

- UI は最新状態を pull するだけにする
- 推論スレッドを UI イベントでブロックしない

## 24. 実装マイルストーン

### M1. 基盤構築

- CMake 初期化
- vcpkg 連携
- アプリ雛形作成

完了条件:

- 空ウィンドウが起動する

### M2. カメラ表示

- Media Foundation キャプチャ実装
- DX11 テクスチャ表示

完了条件:

- カメラ映像が安定表示される

### M3. OpenVINO 導入

- デバイス列挙
- モデル読込
- 単発推論

完了条件:

- NPU / GPU / CPU の利用可否が確認できる

### M4. 顔検出

- 顔検出推論
- バウンディングボックス可視化

完了条件:

- 顔矩形がリアルタイムで表示される

### M5. 頭部姿勢推定

- 顔 crop 作成
- ランドマーク / 姿勢推定

完了条件:

- yaw / pitch / roll が取得できる

### M6. フィルタとマッピング

- OneEuro Filter 導入
- VTuber 向けパラメータ化

完了条件:

- ジッタが減少し、安定した値が出る

### M7. 計測と調整

- UI から各種係数調整
- パフォーマンス計測

完了条件:

- ボトルネックが可視化される

## 25. 今後の拡張余地

- `eye_open` 推定
- `mouth_open` 推定
- 表情ブレンドシェイプ対応
- 複数顔追跡
- Qt UI 化
- 配信用プロトコル出力
- プラグイン化

## 26. 結論

本システムの初期版は、以下の構成が最も現実的である。

- カメラ: `Media Foundation`
- 推論: `OpenVINO Runtime`
- デバイス: `NPU` を第一候補、`GPU`、`CPU` へ明示フォールバック
- モデル: `顔検出 + 顔ランドマーク / 頭部姿勢` の 2 段構成
- フィルタ: `OneEuro Filter`
- 描画: `DirectX11`
- UI: `Dear ImGui`
- 並列: `std::jthread` 中心

この方針により、Intel AI Boost を活用しつつ、低遅延・安定動作・実装容易性のバランスを取りやすい。特に初期フェーズでは、NPU に適合する `固定 shape` の軽量モデルを前提とし、最新フレーム優先のリアルタイムパイプラインを維持することが重要である。
