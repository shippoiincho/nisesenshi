# 偽戦士のカートリッジ for PC-6001mk2/SR

![nisesenshi on 6001mk2SR](/pictures/board2.jpg)

## これはなに？

PC-6001mk2/SR (PC-6601/SR) 用拡張カートリッジです。
以下の機能があります。

- 拡張漢字 ROM (PC-6601-01/6007SR 相当)
- 戦士カートリッジ互換メガ ROM

[BAKUTENDO さんのユニバーサル基板](https://bakutendo.net/blog-entry-428.html)
を使用するので、拡張 RAM の機能はそちらを使用します。
Pico には入っていません。

---
## 回路

![board](/pictures/board.jpg)

Z80 Bus に直結するために、
ピンの多い RP2350B のボード(WeAct RP2350B)を使用します。
カートリッジスロットの信号を、単純に Raspberry Pi Pico2 に接続しただけです。
Pico2 が 5V 耐性なのを良いことに直結しています。

```
GP0-7:  D0-7
GP8-23: A0-15
GP25: RESET
GP26: CS2
GP27: CS3
GP28: MERQ
GP29: IORQ
GP30: WR
GP31: RD
5V: VUSB
GND: GND
```

ユニバーサル基板上の RAM の配線については、BAKUTENDO さんの説明をご覧ください。

---
## 拡張漢字ROM

![Extended KANJI](/pictures/extkanji.jpg)

拡張漢字ROM のエミュレートをします。
あらかじめ picotool などで漢字 ROM のデータを書き込んでおく必要があります。

BASIC 起動時に内容のチェックを行っているので、純正 ROM のデータが必要です。
(PC-8801 の漢字ROM のデータが使えるという説も？)

```
picotool.exe load -v -x kanji.rom  -t bin -o 0x10060000
```

---
## 戦士カートリッジ互換機能

128KiB のメガロムをエミュレートします。
ポート 70H でバンク切り替えをする、初代戦士カートリッジ(ベルーガカートリッジ)に相当します。

内部的には 64 個のスロットがあって、そのうちの一つが接続されているように見えます。

ROM のデータはあらかじめフラッシュに書き込んでおく必要があります。
アドレス 0x10080000 から 128KiB 単位でおいてください。
最初(0番)の ROM 以下のように書き込みます。

```
picotool.exe load -v -x kanji.rom  -t bin -o 0x10080000
```


電源オンの際には、メガロムは無効(0xff)になっています。

メガロムのソフトを使うには、スロットを切り替えてリセットを行います。

メガロムの切り替えには BASIC の OUT 命令を使用します。
1番の ROM に切り替えるには BASIC から以下のようにします。

```
OUT &H77,1
```

切り替えたのちにリセットボタンを押すと ROM から起動します。

あまり意味はありませんが、現在の ROM の番号は以下の方法で取得できます。

```
PRINT INP(&H77)
```
