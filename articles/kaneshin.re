
= iotaの使い方

== はじめに

株式会社エウレカ@<fn>{ks_intro_eureka}でPairs@<fn>{ks_intro_pairs}の開発や、エンジニア組織のマネジメントをしているkaneshin@<fn>{ks_intro_kaneshin}です。大学の専攻や前職ではC言語がメインだったため、C言語での開発が一番得意なのと、数学についても大好きです。Go言語では主に、CLIツールやAPIサーバーを開発することが多いです。
//footnote[ks_intro_eureka][@<href>{https://eure.jp/}]
//footnote[ks_intro_pairs][@<href>{https://www.pairs.lv/}]
//footnote[ks_intro_kaneshin][@<href>{https://twitter.com/kaneshin0120}]

本章では、Go言語の定数宣言内の識別子である@<tt>{iota}についての説明と、ビット演算と組み合わせて活用する解説をします。

== iotaとは

@<tt>{iota}とは、ギリシャ文字の９番目のιにあたり、発音は『ァイオゥタ』です。
日本人の多くは、イオタと発音する人が多いと思います。発音の『ァイオゥタ』の小文字で表記している発音が、発音リダクションによって脱落しているからです。

@<tt>{iota}は@<strong>{型なしの連続する整数定数}を暗黙的に展開する列挙子です。C言語の@<tt>{enum}のような列挙型ではなく、@<tt>{const}の宣言グループ内で使用し、セットされた@<tt>{iota}へ値が@<tt>{0}からはじまる連続整数が展開されます。

//list[ks_code_how_to_use_iota][iotaの連続した整数の展開][go]{
const (
  GolangTokyo  = iota // 0
  GoConference = iota // 1
  GopherCon    = iota // 2
)
//}

@<tt>{iota}は新しく@<tt>{const}のグループであれば、@<tt>{0}から新しく展開されますし、同じ@<tt>{const}グループ内であれば、@<tt>{iota}を省略した場合、コンパイル時に暗黙的にコードの文脈で補完されて展開されます。

//list[ks_code_split_const_group][constを分けるとiotaは0から展開][go]{
// 1st const group
const (
  GolangTokyo  = iota // 0
  GoConference = iota // 1
  GopherCon    = iota // 2
)

// 2nd const group
const (
  DroidKaigi = iota // 0
  TrySwift          // 1
  PyCon             // 2
)
//}

=== ConstSpec評価でインクリメント

@<tt>{iota}は@<tt>{0}からはじまる連続整数の値で展開されますが、インクリメントされるタイミングはコンパイル時に@<tt>{ConstSpec}が評価されるタイミングでインクリメントを行っています。そのため、識別子と式のリストである@<tt>{IdentifierList, ExpressionList}が同一の@<tt>{ConstSpec}に存在する場合、同一の整数値が割り当てられます。

簡単に言えば、この@<tt>{ConstSpec}は@<tt>{const}をコンパイル時に一行ずつ解釈するものです。つまり、一行ずつ整数値が定義されることになります。

//list[ks_code_multiple_assign_iota][一行ずつiotaは展開][go]{
const (
  Foo1, Foo2 = iota, 2 * iota // 0, 0 (iota == 0)
  Bar1, Bar2                  // 1, 2 (iota == 1)
  Qux1, Qux2                  // 2, 4 (iota == 2)
)
//}

=== ブランク識別子でスキップ

ブランク識別子とは、@<tt>{_}（アンダースコア）を使用して表現します。Goの標準パッケージにて、@<tt>{database/sql/driver}を満たしたDriverをアンダースコアをつけてインポートした経験がある人もいると思います。

//list[ks_code_blank_import][blank importの例][go]{
import (
  "database/sql"
  _ "github.com/go-sql-driver/mysql"
)
//}

このブランク識別子を@<tt>{const}グループ内で使用すると、@<tt>{ConstSpec}として解釈されますが、宣言において@<tt>{IdentiferList}と@<tt>{ExpressionList}へバインドはしません。そのため、コンパイル時に@<tt>{ConstSpec}の解釈で@<tt>{iota}のインクリメントが実行されることになっています。

//list[ks_code_blank_iota][ブランク識別子でスキップ][go]{
const (
  Foo1, Foo2 = iota, 2 * iota // 0, 0 (iota == 0)
  _, _                        // _, _ (iota == 1)
  Qux1, Qux2                  // 2, 4 (iota == 2)
)
//}

==== ConstSpec, IdentifierList, ExpressionList

本章の、@<tt>{const}の話とは直接関係ありませんが、@<tt>{ConstSpec, IdentifierList, ExpressionList}は、コンパイラが事前宣言における解釈をするためのリストです。コンパイラの方で@<tt>{type}や@<tt>{const}の宣言を判断し、対応する@<tt>{TypeSpec}や@<tt>{ConstSpec}に適切にアサインしています。

//list[ks_code_compiler_spec_list][ConstSpec, Identifier, Expressionのリスト][go]{
ConstDecl      = "const" ( ConstSpec | "(" { ConstSpec ";" } ")" ) .
ConstSpec      = IdentifierList [ [ Type ] "=" ExpressionList ] .

IdentifierList = identifier { "," identifier } .
ExpressionList = Expression { "," Expression } .
//}

@<tt>{IdentifierList, ExpressionList}は、それぞれ「識別子」と「式」のリストになっており、カンマ区切りのものをバインドします。@<fn>{ks_constant_declarations}
//footnote[ks_constant_declarations][@<href>{https://golang.org/ref/spec#Constant_declarations:title}]

== iotaの活用例

本家の@<tt>{iota}のページ@<fn>{ks_golang_wiki_iota}に載っている例で解説します。
//footnote[ks_golang_wiki_iota][@<href>{https://github.com/golang/go/wiki/Iota:title}]

//list[ks_code_const_byte_size][バイトサイズをiotaで定義][go]{
type ByteSize float64

const (
  _           = iota // ignore first value by assigning to blank identifier
  KB ByteSize = 1 << (10 * iota)
  MB
  GB
  TB
  PB
  EB
  ZB
  YB
)
//}

ブランク識別子をつかって最初の値はスキップするように実装しています。ブランク識別子を使わないでも実装することは可能です。

//list[ks_code_const_byte_size_without_blank][バイトサイズをiotaで定義（ブランク識別子なし）][go]{
const (
  KB ByteSize = 1 << (10 * (iota + 1)) // 1 << 10
  MB                                   // 1 << 20
  GB                                   // 1 << 30
  // ...
)
//}

@<list>{ks_code_const_byte_size_without_blank}では、@<tt>{1 << (10 * (iota + 1))}と記述する必要がありますが、@<tt>{1 << (10 * iota + 1)}と記述してしまったら期待動作通りにはなりません。演算子の評価の優先順位を踏まえれば、当たり前の挙動ですが、不具合の温床となってしまいます。このような観点で、ブランク識別子を有効活用すると不具合を発生させないコードになります。

=== 数値に意味がある定数への使用

連続整数として定数に数値を展開することが@<tt>{iota}の機能であることが理解できたと思います。そのため、連続する数値に対して活用できると思いますが、数値として意味を持ってしまっている定数へは使用しないことが望ましいです。例えば、データーベースに入っている連続整数のマスターレコードの値を定数として定義する場合です。

//list[ks_code_mysql_select][データベースのレコード][go]{
mysql> SELECT * FROM conference_type;
+----+--------------------+
| id | conference_type    |
+----+--------------------+
|  1 | public_conference  |
|  2 | private_conference |
|  3 | other_conference   |
+----+--------------------+
//}

これをGoのコードで表現するために@<tt>{iota}で設定するのは、あまり良い方法とは言えません。データーベースに数値が設定されているということは、既に、その数値は「データーベースに存在する値」として強い意味を持っていることになります。Goの定数の順番を間違えて変更してしまった場合、データーベースの値と違うものになってしまいます。

//list[ks_code_const_conference_type_example][データーベースの値を定数で設定][go]{
type ConferenceType int

// 好ましい例
const (
  PublicConference  ConferenceType = 1
  PrivateConference ConferenceType = 2
  OtherConference   ConferenceType = 3
)

// 好ましくない例
const (
  PublicConference  ConferenceType = iota + 1 // 1
  PrivateConference                           // 2
  OtherConference                             // 3
)
//}

連続整数だからと、すべてに@<tt>{iota}を使わない設計を考えることも重要です。

=== ビット演算との組み合わせ

さて、数値に意味がないものに対しては積極的に@<tt>{iota}を使っていきます。例えば、Pairsのユーザーの状態を管理をするために、ビット処理と組み合わせたとすれば、@<tt>{iota}を使った定数は相性が良いものの一つとなります。

//list[ks_code_const_iota_bitwise][ビット演算とiota][go]{
package main

import (
  "fmt"
)

type RelationshipStatus int

const (
  LikeFromMe       RelationshipStatus = 1 << iota // 0001 (1 << 0)
  LikeFromPartner                                 // 0010 (1 << 1)
  BlockFromMe                                     // 0100 (1 << 2)
  BlockFromPartner                                // 1000 (1 << 3)
)

const (
  MutualLike = LikeFromMe | LikeFromPartner   // 0011
  Blocking   = BlockFromMe | BlockFromPartner // 1100
)

func (st RelationshipStatus) IsMatching() bool {
  // お互いのいいね！判定
  // MutualLikeのビットが立っているかをAND演算で判定
  if st&MutualLike != MutualLike {
    return false
  }

  // ブロックしている場合はマッチングにしない判定（案1、2どちらでも）
  // 案1: Blockingのビットが立っているかをAND演算で判定
  if st&Blocking != 0 {
    return false
  }
  // 案2: お互いのいいね！をAND NOT演算でクリアして判定
  if st&^MutualLike != 0 {
    return false
  }

  return true
}
//}

読みやすいようにビットを記載していますが、このコードを実行する場合、定数に入っている数値自体には意味がありません。昨今、ビット演算を使うことは少ないですが、覚えていて損は無いテクニックです。

== おわりに

本章では、何気なく使う@<tt>{iota}について、@<tt>{ConstSpec}のコンパイル解釈を含めて解説しました。@<tt>{ConstSpec}のように、コンパイル時の解釈を踏まえておくことによって、@<tt>{iota}への深い理解へと繋がります。C言語やSwiftの@<tt>{enum}のようなことはできませんが、Go言語ならではの使い方として、使用していってもらえればと思います。
