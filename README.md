# CompactMeter

Windows用のCPU使用率やネット通信量をメーター表示するウィジェットっぽいツールです。

Direct2D 対応なのでCPU負荷を低く抑えています。

![SS](image/2018-12-19_16h24_46.gif)


## ダウンロード

- [Releases](https://github.com/takke/CompactMeter/releases) からダウンロードできます。
- [AppVeyorのartifacts](https://ci.appveyor.com/project/takke/compactmeter/build/artifacts) から最新版(スナップショット)をダウンロードできます。


## 対応OS

Windows7, Windows8.1, Windows10


## 操作方法

| 操作 | 説明 |
| --- | --- |
| ウィンドウをドラッグ | 移動 |
| Shiftキーを押しながらドラッグ | サイズ変更 |
| 右クリック | メニュー表示 |
| ESCキー、F12キー | 終了 |
| Tキー | 「常に最上位に表示」切替 |
| Bキー | 枠線表示の切替 |
| Dキー | デバッグモード切替 |


## 変更履歴

### v1.1.0 (2019.01.xx)
- 各ドライブのI/Oメーター追加
- ウィンドウの最小サイズを 300x300 -> 200x200 に変更
- メーター設定追加、順序変更と表示除外に対応
- コア別メーター設定を削除(メーター設定に統合)


### v1.0.0 (2018.12.27)

- 初版リリース
