# 軌跡パターン推薦のためのFrequent Path Tree

[English README](README.md)

本リポジトリは、階層ビットマップを用いた軌跡パターン索引と、
類似度に基づく経路推薦を実装したC++17ポートフォリオプロジェクトです。

北京工業大学コンピュータ科学・技術学科における
個人卒業研究を基に、当時のVisual Studio向け試作コードを、
CMakeでビルド可能な再現性のある構成へ整理しました。

---

## 概要

大量の移動軌跡パターンを扱う場合、
各パターンを固定長ビットマップとして保存すると、
データが疎であっても大きな保存領域が必要になります。

本プロジェクトでは、従来のSignature Treeで使用される
固定長ビットマップを、必要な部分のみを生成する
3階層ビットマップへ置き換えました。

この階層ビットマップをFrequent Path Tree（FPT）に組み込み、
以下の処理を実現しています。

- 疎な軌跡パターンの効率的な保存
- 軌跡パターンの木構造への挿入
- 容量超過時の再帰的なノード分割
- 問い合わせ軌跡との類似度検索
- 支持度を利用した候補選択
- Signature Treeとの性能比較

---

## プロジェクト背景

本プロジェクトは、北京工業大学の卒業研究として
個人で取り組んだものです。

研究目的は、大規模な軌跡パターン集合に対して、
保存領域と検索コストを削減することでした。

元の研究ではC++とVisual Studioを使用して
学術的な試作プログラムを構築しました。

本リポジトリでは、その設計を保ちながら、
以下の点をポートフォリオ向けに再整理しています。

- C++17対応
- CMakeによるクロスプラットフォームビルド
- クラスとファイル構成の明確化
- 再現可能なサンプルデータ
- FPTとSignature Treeの実装分離
- 自動整合性チェック
- デモ結果と卒業研究結果の分離

---

## 使用技術

- C++17
- CMake
- データ構造とアルゴリズム
- 木構造インデックス
- 階層ビットマップ
- Signature Tree
- 軌跡類似度検索
- アルゴリズム性能評価
- Python
- Matplotlib

PythonとMatplotlibは、
卒業研究の評価結果をグラフとして可視化するために使用しました。

---

## 担当内容

本プロジェクトは個人卒業研究です。

主に以下の作業を担当しました。

- 軌跡パターン索引手法の調査
- Signature TreeベースラインのC++実装
- 3階層ビットマップの設計と実装
- 階層ビットマップとFPTの統合
- 軌跡パターン挿入処理の実装
- 再帰的なノード分割処理の実装
- 類似軌跡検索処理の実装
- 支持度を利用した同点候補の選択
- シミュレーション用軌跡データの生成
- 保存領域、構築時間、検索時間の比較
- 実験結果の分析
- 卒業論文の作成
- 元のVisual StudioコードのC++17・CMake化

---

## システム概要

![システム概要](figures/system_overview.png)

システムは主に以下の流れで動作します。

```text
軌跡パターンデータ
        ↓
FPTまたはSignature Treeを構築
        ↓
問い合わせ軌跡を入力
        ↓
木構造を探索
        ↓
類似度を計算
        ↓
支持度を用いて候補を比較
        ↓
最も類似した軌跡パターンを推薦
```

---

## 中心となる考え方

本実装では、道路区間IDが次の範囲に存在すると仮定しています。

```text
50 × 50 × 50 = 125,000
```

従来の固定長ビットマップでは、
各軌跡パターンに対して125,000個分のビット領域を確保します。

しかし、1つの軌跡が実際に使用する道路区間は、
その一部に限られます。

そこで、道路区間IDを3つの階層へ分割します。

```text
road_segment_id
        ↓
root index
        ↓
middle index
        ↓
leaf index
```

必要なmiddleノードとleafノードのみを動的に生成することで、
疎な道路区間IDの保存コストを削減します。

![階層ビットマップ](figures/hierarchical_bitmap.png)

---

## Frequent Path Tree

階層ビットマップは、
Frequent Path Treeの各ノード内で使用されます。

内部ノードは、子孫ノードが持つ階層ビットマップの
和集合を保持します。

問い合わせ軌跡と重ならないビットマップを持つ枝は、
探索対象から除外できます。

これにより、すべての軌跡パターンを
逐次比較する必要を減らします。

![FPT構造](figures/fpt_structure.png)

---

## リポジトリ構成

```text
trajectory-pattern-index-tree/
├── CMakeLists.txt
├── README.md
├── README_ja.md
├── figures/
│   ├── build_time_comparison.png
│   ├── fpt_structure.png
│   ├── hierarchical_bitmap.png
│   ├── response_time_comparison.png
│   ├── storage_comparison.png
│   └── system_overview.png
├── results/
│   ├── demo_output.txt
│   └── thesis_benchmark.csv
├── sample_data/
│   ├── query_trajectory.csv
│   └── trajectory_patterns.csv
└── src/
    ├── frequent_path_tree.hpp
    ├── hierarchical_bitmap.hpp
    ├── main.cpp
    ├── route_recommender.hpp
    ├── signature_tree.hpp
    └── trajectory_pattern.hpp
```

---

## 主要コンポーネント

### `hierarchical_bitmap.hpp`

道路区間IDを保存する、
疎な3階層ビットマップを実装しています。

主な処理：

- 道路区間IDの挿入
- IDが存在するかの判定
- 2つのビットマップの共通部分計算
- 2つのビットマップの和集合計算
- 生成済みノードが使用する保存領域の推定

### `frequent_path_tree.hpp`

Frequent Path Treeを実装しています。

主な処理：

- 軌跡パターンの挿入
- 挿入先となる子ノードの選択
- 容量を超えたノードの分割
- 集約ビットマップの再構築
- 問い合わせと無関係な枝の除外
- 最も類似する軌跡パターンの返却

### `signature_tree.hpp`

125,000ビットの固定長ビットマップを使用する
Signature Treeベースラインを実装しています。

FPTと同じ木構築・推薦インターフェースを使用することで、
同じ条件下で両手法を比較できます。

### `route_recommender.hpp`

問い合わせ軌跡と候補軌跡の
重み付きカバレッジを計算します。

問い合わせ軌跡の後方に位置する要素ほど、
大きい重みを付けます。

```text
1, 10, 100, 1000, ...
```

類似度が最も高い候補を選択し、
類似度が同じ場合は支持度が高い候補を優先します。

### `trajectory_pattern.hpp`

軌跡パターンのID、支持度、
道路区間列などの基本データを管理します。

### `main.cpp`

CSVファイルの読み込み、インデックスの構築、
問い合わせの実行、結果表示、
整合性チェックを行います。

---

## サンプルデータ

### `sample_data/trajectory_patterns.csv`

```csv
pattern_id,support,road_segments
P001,0.50,2 3 9 25 60
P002,0.80,3 5 9 23 61
P003,0.40,2 3 23 30 60
P004,0.70,2 3 9 60
```

各行には、以下の情報が含まれます。

- パターンID
- 支持度
- 道路区間IDの系列

### `sample_data/query_trajectory.csv`

```csv
query_id,road_segments
Q001,2 5 9 60
```

この問い合わせに対して、
FPTとSignature Treeの両方が`P004`を推薦します。

---

## ビルドと実行

### 必要環境

- C++17対応コンパイラ
- CMake 3.16以降

### LinuxまたはmacOS

```bash
cmake -S . -B build
cmake --build build
./build/trajectory_demo
```

### Windows

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\trajectory_demo.exe
```

独自の入力ファイルも指定できます。

```bash
./build/trajectory_demo         path/to/trajectory_patterns.csv         path/to/query_trajectory.csv
```

---

## デモ出力

```text
Trajectory Pattern Index Demo
=============================
Loaded patterns: 4
Query Q001: 2 5 9 60

FPT recommendation:
P004 | similarity=0.9910 | support=0.70 | route=2 3 9 60

Signature Tree recommendation:
P004 | similarity=0.9910 | support=0.70 | route=2 3 9 60

Index summary
-------------
FPT nodes/depth: 3/2
Signature Tree nodes/depth: 3/2
FPT estimated bitmap bytes: 224
Signature Tree estimated bitmap bytes: 109424
All checks passed.
```

実行時間は使用する環境によって変化するため、
マイクロ秒単位の値は参考例として
`results/demo_output.txt`に保存しています。

---

## 卒業研究の評価結果

以下は、小規模なデモを実行した結果ではなく、
元の卒業研究でシミュレーションデータを使用して得た結果です。

詳細な値は次のファイルに保存しています。

```text
results/thesis_benchmark.csv
```

### 保存領域

10,000件の軌跡パターンを使用した場合：

| インデックス | 報告された保存領域 |
|---|---:|
| Frequent Path Tree | 202,294 KB |
| Signature Tree | 304,962 KB |

![保存領域比較](figures/storage_comparison.png)

### インデックス構築時間

10,000件の軌跡パターンを使用した場合：

| インデックス | 報告された構築時間 |
|---|---:|
| Frequent Path Tree | 70.99秒 |
| Signature Tree | 13,783.93秒 |

両手法の差が大きいため、
グラフの縦軸には対数スケールを使用しています。

![構築時間比較](figures/build_time_comparison.png)

### 問い合わせ応答時間

10,000件の軌跡パターンを使用した場合：

| インデックス | 報告された応答時間 |
|---|---:|
| Frequent Path Tree | 0.085秒 |
| Signature Tree | 3.809秒 |

![応答時間比較](figures/response_time_comparison.png)

---

## 本プロジェクトで示したスキル

本プロジェクトを通して、以下の能力を示しています。

- 索引問題を独自のC++データ構造へ変換する能力
- 木構造への挿入と再帰的ノード分割の実装
- 疎なデータ表現による保存コスト削減
- 比較可能なFPT・Signature Treeインターフェースの設計
- 再現可能なサンプル入出力の作成
- 保存領域、構築時間、検索時間の評価
- デモ結果と卒業研究結果の明確な区別
- 学術的アルゴリズムを実行可能な作品として整理する能力

---

## 元の試作コードからの改善点

元の卒業研究用プログラムはVisual Studio上で開発し、
実装の大部分を少数のヘッダーファイルにまとめていました。

本ポートフォリオ版では、以下の改善を行いました。

- C++17インターフェースへの整理
- CMakeによるクロスプラットフォーム対応
- 内容を表すファイル名とクラス名への変更
- 決定的なサンプル入力の追加
- FPTとSignature Treeの実装分離
- 自動整合性チェックの追加
- Windows専用監視コードの削除
- 元の無限待機ループの削除
- デモ結果と卒業研究結果の分離

---

## 公開範囲と制限

本リポジトリは、登録済みの軌跡パターン集合から、
問い合わせに最も類似する完全な軌跡パターンを推薦します。

以下の機能は含まれていません。

- 道路区間と実際の地図との接続
- 最短経路の計算
- 交通状況の予測
- 次の1道路区間のみを直接予測する機能
- 卒業研究規模の評価実験の完全な自動再現

公開しているCSVファイルは、
インデックス構築と推薦処理を確認するために作成した
小規模な人工サンプルです。

---

## 学術的背景

- プロジェクト種別：個人卒業研究
- 専攻：コンピュータ科学・技術
- 大学：北京工業大学
- 元の開発環境：C++、Visual Studio
- ポートフォリオ版：C++17、CMake

本研究では、Signature Tree型インデックスと
疎な階層ビットマップを組み合わせることで、
大規模な軌跡パターン集合における
保存コストと検索コストの削減を目指しました。

---

## 備考

本リポジトリは、卒業研究で設計・実装したアルゴリズムを、
採用担当者や開発者が確認しやすい形へ整理した
就職活動用ポートフォリオです。

小規模なサンプルデータとCMakeによるビルド環境を提供し、
主要なデータ構造、検索処理、推薦結果を再現できるようにしています。
