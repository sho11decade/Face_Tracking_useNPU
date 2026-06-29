# 推論実装

## 概要

現在の `inference` モジュールは、OpenVINO 実推論の前段として以下を担っています。

- デバイス選択
- モデルカタログ管理
- モデルファイル解決
- フレームからの顔観測値生成

`NPUVT_ENABLE_OPENVINO=ON` の場合は、`Core::read_model()`、`compile_model()`、`InferRequest` を使った顔検出の実行経路が有効になります。既定ビルドでは `OFF` のため、簡易推定へフォールバックします。

## 公開 API

`include/npu_vtube/inference/inference_service.hpp` の主な API は次の通りです。

```cpp
struct InferenceOptions {
    std::vector<std::string> preferred_devices {"NPU", "GPU", "CPU"};
    std::string model_path;
    std::string device_hint;
    std::string models_root {"models"};
    bool prefer_onnx {true};
};

struct ModelLoadSummary {
    npuvt::models::ResolvedModel face_detection;
    npuvt::models::ResolvedModel facial_landmarks;
    npuvt::models::ResolvedModel head_pose;
    bool face_detection_ready {false};
    bool facial_landmarks_ready {false};
    bool head_pose_ready {false};
};

class InferenceService {
public:
    std::vector<std::string> query_available_devices();
    bool initialize_runtime(std::string* error_message = nullptr);
    InferenceRuntimeStatus runtime_status() const;
    std::string select_device(const std::vector<std::string>& available_devices) const;
    const npuvt::models::ModelCatalog& model_catalog() const noexcept;
    npuvt::models::ModelResolver model_resolver() const;
    std::optional<npuvt::models::ResolvedModel> resolve_model(npuvt::models::ModelRole role) const;
    ModelLoadSummary load_model_summary() const;
    npuvt::core::FaceObservation analyze(const npuvt::core::FramePacket& frame);
    npuvt::core::FaceObservation make_placeholder_observation() const;
};
```

## 現在の動作

### `select_device()`

- `device_hint` が指定されていれば優先
- それ以外は `preferred_devices` の順に選択
- 実際の `NPU/GPU/CPU` 列挙結果との照合は呼び出し側で行う想定

### `model_catalog()`

- 推奨モデル定義を返す
- 現在のカタログは `docs/models.md` の選定と一致する

### `model_resolver()` / `resolve_model()`

- `models_root` 配下から `xml/bin/onnx` を探す
- 実ファイルが無くても、解決結果を返せる
- 後続の OpenVINO 読み込み前に存在確認できる

### `load_model_summary()`

- 顔検出
- 顔ランドマーク
- 頭部姿勢

の 3 つをまとめて解決し、どれが利用可能かを返す。

### `initialize_runtime()`

- OpenVINO が有効なら `Core` を初期化
- 利用可能デバイス一覧を取得
- 顔検出モデルを読み込み、選択デバイスへ `compile_model()` する
- 失敗時は `runtime_status().last_error` へ理由を残す

### `analyze()`

- OpenVINO 有効かつ顔検出モデルがコンパイル済みなら実推論を行う
- `bgra8` / `bgr8` / `rgb8` を NCHW へ前処理して入力テンソルを作る
- OpenVINO 無効、モデル未配置、推論失敗時は簡易 bbox 推定へフォールバックする

## 現在の制限

- 既定ビルドでは OpenVINO が無効
- この環境では OpenVINO パッケージ未導入のため `ON` 構成は未検証
- `PrePostProcessor`、`AsyncInferQueue` は未使用
- デバイスの実コンパイル結果に基づくフォールバックは未実装
- 顔検出以外のランドマーク/頭部姿勢はまだ未接続

## 次の実装ステップ

1. `face-detection-0200` を OpenVINO で読み込む
2. `landmarks-regression-retail-0009` と `head-pose-estimation-adas-0001` を段階的に接続する
3. 実デバイスの `NPU -> GPU -> CPU` フォールバックを実装する
4. `PrePostProcessor` や async 実行を導入する
5. 実行統計を `PerfSnapshot` と UI に接続する
