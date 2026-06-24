# キャプチャ実装

## 概要

現在の `capture` モジュールは、Windows の Media Foundation を使って

- 映像入力デバイスを列挙する
- 指定デバイスを開く
- 1 フレームを同期取得する

ところまで実装されています。

連続キャプチャ、非同期パイプライン、フレームキューはまだ入っていません。現状は「カメラ入出力の土台確認」が主目的です。

## 公開 API

`include/npu_vtube/capture/capture_service.hpp` の API は次の通りです。

```cpp
struct CaptureDeviceInfo {
    std::string name;
    std::string symbolic_link;
};

struct CaptureOptions {
    std::uint32_t device_index {0};
    std::uint32_t width {1280};
    std::uint32_t height {720};
    npuvt::core::PixelFormat preferred_format {npuvt::core::PixelFormat::bgra8};
};

class CaptureService {
public:
    std::string_view describe() const noexcept;
    std::vector<CaptureDeviceInfo> list_devices() const;
    bool initialize(const CaptureOptions& options, std::string* error_message = nullptr);
    void shutdown() noexcept;
    bool is_initialized() const noexcept;
    npuvt::core::FramePacket grab_frame(std::string* error_message = nullptr);
    npuvt::core::FramePacket make_placeholder_frame() const;
};
```

## 現在の動作

### `list_devices()`

- `MFEnumDeviceSources` を使って映像入力デバイスを列挙
- フレンドリ名と symbolic link を返す
- 列挙時は一時的に COM と Media Foundation を初期化してから終了する

### `initialize()`

- `CoInitializeEx` と `MFStartup` を実行
- 指定 `device_index` の `IMFMediaSource` を開く
- `IMFSourceReader` を作成
- ネイティブの映像形式を総当たりし、要求 `width` / `height` に最も近いものを選ぶ
- その後、出力形式を `RGB24` または `RGB32` に設定する

実装上の対応:

- `PixelFormat::rgb8` -> `MFVideoFormat_RGB24`
- それ以外の既定経路 -> `MFVideoFormat_RGB32`

### `grab_frame()`

- `IMFSourceReader::ReadSample` を同期呼び出し
- `IMFSample` を連続バッファ化して生バイト列を `FramePacket::image.bytes` にコピー
- `frame_id` は内部カウンタで採番
- `timestamp` は Media Foundation 由来ではなく `std::chrono::steady_clock::now()` を格納

### `shutdown()`

- `IMFSourceReader` を解放
- `MFShutdown` と `CoUninitialize` を実行

## `tracker_dev` での利用

`apps/tracker_dev/main.cpp` では次の最小確認をしています。

1. デバイス一覧を表示
2. 先頭カメラを `initialize()`
3. `grab_frame()` で 1 フレーム取得
4. 解像度とバイト数を標準出力に表示
5. `shutdown()`

このため、現時点の `capture` は「単発の疎通確認」はできますが、リアルタイムループの部品としてはまだ未完成です。

## 現在の制限

- Windows 専用。非 Windows ではプレースホルダ応答のみ
- 同期読み出しのみで、専用スレッドや有界キューがない
- 解像度選択は幅と高さの差分のみで評価し、FPS や色形式の優先度は見ていない
- 取得フレームは生バイト列のままで、`stride` や詳細メタデータを持たない
- Media Foundation のサンプル時刻は保持していない
- デバイス切断、再接続、ホットプラグへの対処がない
- 複数フレームの連続取得 API がない
- ピクセル形式は `rgb8` / `bgra8` 前提で、`nv12` はまだ未接続

## 次のキャプチャ実装ステップ

近接タスク:

- 連続取得ループを `CaptureService` に追加
- 最新優先のフレーム受け渡しを導入
- 取得失敗時のリトライとエラー分類を整備
- 実サンプル時刻の取り込みを検討
- フォーマット選択時に FPS とサブタイプも考慮

その先のタスク:

- OpenCV 補助経路の追加可否を整理
- キャプチャ設定を `configs/default.json` と接続
- プレビュー表示向けのテクスチャ更新経路へ接続

## 実装済み / 未実装の線引き

実装済み:

- Media Foundation の初期化と終了
- カメラ列挙
- デバイス選択
- 1 フレーム同期取得

未実装:

- 常時ストリーミング
- パイプライン連携用のスレッド/キュー
- キャプチャ統計
- 自動テストしやすい抽象化層
