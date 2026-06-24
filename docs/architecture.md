# アーキテクチャ概要

このリポジトリは、顔トラッキング処理を段階的なモジュールに分けて実装する構成です。現時点では全段が完成しているわけではなく、`capture` のみが実動に近く、以降の段は骨格またはプレースホルダです。

## 現在のデータフロー

`tracker_dev` の現在の流れは次の通りです。

```text
ApplicationInfo
  -> CaptureService.list_devices()
  -> InferenceService.select_device()
  -> InferenceService.make_placeholder_observation()
  -> TrackingService.update()
  -> ParameterMapper.map()
  -> RenderBackend.name()
  -> DebugUi.name()
  -> optional: CaptureService.initialize() + grab_frame()
```

設計上の目標パイプラインは `TECHNICAL_DETAILS.md` にある通り

```text
Camera -> Capture -> Inference -> Tracking/Filter -> Mapping -> Render -> UI
```

ですが、実コードではまだ同期的な確認用経路です。

## モジュール境界

| モジュール | 主な責務 | 現在の状態 |
| --- | --- | --- |
| `core` | 共通型、アプリ情報 | 実装済み |
| `capture` | カメラ列挙、初期化、フレーム取得 | 実装済み |
| `inference` | 推論デバイス選択、顔観測生成 | プレースホルダ |
| `tracking` | 観測値から追跡状態を生成 | 最小実装 |
| `mapping` | 追跡状態を VTuber 向け値へ変換 | 最小実装 |
| `render/dx11` | プレビューとオーバーレイ描画 | スタブ |
| `ui/imgui` | デバッグ UI | スタブ |
| `platform/windows` | Windows 環境依存の判定 | 最小実装 |

## 主要データ型

`include/npu_vtube/core/types.hpp` に現在の共通データ型があります。

- `FramePacket`: キャプチャした 1 フレーム。`frame_id`、`timestamp`、解像度、`PixelFormat`、生バイト列を保持
- `FaceObservation`: 推論結果の受け皿。顔矩形、ランドマーク、yaw/pitch/roll、confidence を保持
- `TrackingState`: 観測値から追跡系で扱う状態。顔中心、スケール、姿勢角を保持
- `MappedParameters`: 出力向けの最終値。現状はほぼ素通し
- `PerfSnapshot`: 将来の計測表示用の構造体。まだ本格利用はしていない

## 現在の依存方向

依存は大きく次の向きです。

```text
apps/tracker_dev
  -> core
  -> capture
  -> inference
  -> tracking
  -> mapping
  -> render/dx11
  -> ui/imgui
  -> platform/windows
```

各ライブラリはヘッダを `include/npu_vtube/...` に置き、`src/*` 以下で静的ライブラリとして分割されています。

## 実装済み部分の要点

`capture`:

- Windows では Media Foundation を直接利用
- デバイス列挙と `IMFSourceReader` ベースの読み出しを実装
- 出力ピクセル形式は `rgb8` または `bgra8` を要求可能

`inference`:

- 実モデル読み込みはまだ無い
- NPU 優先のデバイス選択ロジックだけ先に存在

`tracking` / `mapping`:

- 顔矩形中心とスケール計算のみ
- 正規化、外れ値除去、平滑化は未実装

## 現時点で未接続の設計要素

`TECHNICAL_DETAILS.md` に記載されている次の要素は、まだコード上で接続されていません。

- OpenVINO 実推論
- `NPU/GPU/CPU` の実コンパイルとフォールバック
- OneEuro Filter
- 有界キューによる最新優先パイプライン
- DirectX11 テクスチャ更新とオーバーレイ描画
- Dear ImGui によるデバッグパネル

このため、現状のアーキテクチャ理解では「将来の段構成」と「今動いている最小骨格」を分けて見るのが実務上重要です。
