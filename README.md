# 偽戦士のカートリッジ for PC-6001mk2/SR

![nisepac2 on PASOPIA7](/pictures/onpasopia.jpg)

## これはなに？

PC-6001mk2/SR 用拡張カートリッジです。
以下の機能があります。

- 拡張漢字 ROM
- 戦士カートリッジ互換メガ ROM

BAKUTENDO さんのユニバーサル基板を使用するので、拡張 RAM の機能はそちらを使用します。
Pico には入っていません。

---
## 回路

RP2350B のボード(WeAct RP2350B)を使用します。
カートリッジスロットの信号を、単純に Raspberry Pi Pico2 に接続しただけです。
Pico2 が 5V 耐性なのを良いことに直結しています。

ユニバーサル基板上の RAM の配線については、　　　　　　をご覧ください。

---
## 拡張漢字ROM

![KANJI PAC](/pictures/kanjipac.jpg)

拡張漢字ROM のエミュレートをします。
あらかじめ picotool などで漢字 ROM のデータを書き込んでおく必要があります。

```
picotool.exe load -v -x kanji.rom  -t bin -o 0x10060000
```

---
## 戦士カートリッジ互換機能

![RAMPAC](/pictures/rampac.jpg)

128KiB のメガロムをエミュレートします。
内部的には xxx 個のスロットがあって、そのうちの一つが接続されているように見えます。

電源オンの際には、メガロムは無効(0xff)になっています。

メガロムのソフトを使うには、スロットを切り替えてリセットを行います。

メガロムの切り替えには BASIC の OUT 命令を使用します。

```
OUT &H71,1
```


```
PRINT INP(&H71)
```

