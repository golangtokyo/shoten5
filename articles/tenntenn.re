= Goクイズと解説

== はじめに

メルペイ バックエンドエンジニアの@tenntenn@<fn>{tenntenn}です。
本章では、2018年10月04日に行われたMercari Tech Conf 2018@<fn>{mtc18}のメルカリ エキスパートチームブースで展示したGoクイズについて解説を書きたいと思います。

//footnote[tenntenn][@<href>{https://github.com/tenntenn}]
//footnote[mtc18][@<href>{https://techconf.mercari.com/2018}]

== 埋め込みとメソッド

=== 問題

//list[embedded_method][埋め込みとメソッド][go]{
package main

type Hoge struct{}

func (h *Hoge) A() string {
  return "Hoge"
}

func (h *Hoge) B() {
  println(h.A())
}

type Fuga struct {
  Hoge
}

func (f *Fuga) A() string {
  return "Fuga"
}

func main() {
  var f Fuga
  f.B()
}
//}

@<list>{embedded_method}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{Hoge}と表示される
 3. @<code>{Fuga}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{embedded_method_pg}と、
@<code>{Hoge}と表示されるため、答えは2となります。

//footnote[embedded_method_pg][@<href>{https://play.golang.org/p/ShgVQOB3yXb}]

構造体の埋め込みをJavaなどの"継承"と同じだと理解していると、この問題を間違ってしまうでしょう。
構造体の埋め込みは、匿名フィールドという扱いです。

匿名フィールドはフィールド名を持たないため、アクセスする場合は
@<code>{f.Hoge.B()}のように型名でアクセスします。
また、@<code>{f.B()}は@<code>{f.Hoge.B()}と記述した場合と同じように動作するため、
メソッド@<code>{B}内で呼んでいるメソッド@<code>{A}は@<code>{*Hoge}型のものを呼び出しています。

== 組込み型の埋め込み

=== 問題

//list[embedded_builtin][組込み型の埋め込み][go]{
func main() {
  type Hoge struct{ int }
  var h Hoge
  fmt.Println(h)
}
//}

@<list>{embedded_builtin}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{{0}}と表示される
 3. @<code>{{<nil>}}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{embedded_builtin_pg}と@<code>{{0}}と表示されます。

//footnote[embedded_builtin_pg][@<href>{https://play.golang.org/p/1Dp5JBiXEog}]

構造体には任意の名前付きの型を埋め込むことができます。
そのため、@<code>{int}型のような組込み型であっても、名前付きの型であることには変わらないため、
構造体に埋め込むことができます。

また、構造体のゼロ値はすべてのフィールドがゼロ値で初期化された値であるため、匿名フィールドである埋め込みも同様にゼロ値で初期化されています。
そのため、@<code>{Hoge}構造体のゼロ値は匿名フィールドの@<code>{int}がゼロ値である@<code>{0}で初期化された値となります。

== スライスと引数

=== 問題

//list[slice_arg][スライスと引数][go]{
package main

import "fmt"

func f(ns []int) {
  ns[1] = 200
  ns = append(ns, 40, 50)
}

func main() {
  ns := []int{10, 20, 30}
  f(ns)
  fmt.Println(ns)
}
//}

@<list>{slice_arg}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{[10 200 30]}と表示される
 3. @<code>{[10 20 30]}と表示される
 4. @<code>{[10 200 30 40 50]}と表示される

=== 解答と解説


問題のコードを実行してみる@<fn>{slice_arg_pg}と、@<code>{[10 200 30]}と表示されるため、答えは2となります。

//footnote[slice_arg_pg][@<href>{https://play.golang.org/p/NM_KaC2XjiI}]

スライスは配列の一部分を見ているウィンドウのようなものです。
そのため、スライス自体は値を保持せず、@<img>{tenntenn_slice}のように配列の特定の要素へのポインタと、
スライスのサイズである@<code>{len}とスライスをどこまで拡張可能かを表す@<code>{cap}を持ちます。

@<code>{cap}はそのスライスの背後にある配列の大きさによって決まります。
@<img>{tenntenn_slice}の場合は、配列の@<code>{1}番目の要素へのポインタを保持しているため、
そこから拡張可能な大きさは@<code>{4}となります。

//image[tenntenn_slice][スライスの内部構造]{
スライスの内部構造を図示したもの
//}

実際にGoのランタイムではスライスは@<list>{slice_struct}のような構造体で表現されています@<fn>{slice_runtime}。

//list[slice_struct][スライスを表す構造体][go]{
type slice struct {
  array unsafe.Pointer
  len int
  cap int
}
//}

//footnote[slice_runtime][@<href>{https://github.com/golang/go/blob/master/src/runtime/slice.go#L12-L16}]

さて、問題の@<list>{slice_arg}を見ると@<code>{main}関数内で@<code>{ns}というスライスを作り、
関数@<code>{f}に渡しています。
変数@{ns}の値は関数@<code>{f}の引数としてコピーされます。
そのため、@<code>{main}関数の変数@<code>{ns}と関数@<code>{f}の引数@<code>{ns}は別の領域に確保された変数となります。

しかし、スライスは内部に配列へのポインタを持つため、@<code>{ns[1] = 200}という式はポインタ経由で背後にある配列への代入が行われます。
そのため、関数@<code>{f}内の@<code>{ns}と@<code>{main}関数内の@<code>{ns}が持つ配列へのポインタの値は同じであるため、@<code>{main}関数内の@<code>{ns}にも代入の影響を受けます。

一方、関数@<code>{f}内で行われる@<code>{ns = append(ns, 40, 50}について考えてみます。
組込み関数の@<code>{append}はスライスを第1引数に受け取り、第2引数以降で渡された値をそのスライスの後ろに追加していきます。

スライスの容量（@<code>{cap}）が十分に大きい場合、つまり、スライスのサイズ（@<code>{len}）と追加予定の要素の数を足した値が、@<code>{cap}を超えない場合には、@<code>{append}は背後にある配列の要素を追加予定の要素の値で上書きし、スライスの@<code>{len}を更新します。

一方で、@<code>{cap}の大きさを超えるような数の要素を追加する場合は、追加予定の要素がすべて入るように背後にある配列を再確保します。
そのため、@<code>{append}は新しい配列に、スライスが参照しているすべての要素をコピーし、追加予定の要素のコピーを行います。
そして、スライスが保持する配列へのポインタと@<code>{len}、@<code>{cap}の更新を行います。

このように、@<code>{append}では第1引数で渡したスライスが保持する配列のポインタや@<code>{len}、@<code>{cap}などの更新を行います。
そのため、関数@<code>{f}の中で引数@<code>{ns}の値が変更されますが、その変更は呼び出し元の@<code>{main}関数に影響しません。

つまり、関数@<code>{f}内で追加された@<code>{40}と@<code>{50}は、@<code>{main}関数内の@<code>{ns}で追加されたことになりません。
そのため、@<code>{[10 200 30]}と表示されます。

== スライスとdefer

=== 問題

//list[slice_defer][スライスとdefer][go]{
func main() {
  ns := []int{10, 20, 30}
  defer fmt.Println(ns)
  ns[1] = 200
  ns = append(ns, 40, 50)
}
//}

@<list>{slice_defer}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{[10 20 30]}と表示される
 3. @<code>{[10 200 30]}と表示される
 4. @<code>{[10 200 30 40 50]}と表示される

=== 解答と解説

問題のコードを実行してみる@<fn>{slice_defer_pg}と、@<code>{[10 200 30]}と表示されるため、答えは3となります。

//footnote[slice_defer_pg][@<href>{https://play.golang.org/p/UNXGfcDL8Nn}]

@<code>{defer}で実行される関数の引数は@<code>{defer}が記述されている時点での値が用いられます。
そのため、@<list>{slice_defer}では@<code>{defer}のあとに変数@<code>{ns}を書き換えても@<code>{main}関数終了時に@<code>{fmt.Println}を実行する際には影響を受けません。

しかし、前述の通り、スライスは内部に配列へのポインタを持つため、@<code>{ns[1] = 200}における代入はそのポインタ経由で行われるため出力される値は@<code>{[10 200 30]}となります。

== ポインタとメソッドセット

=== 問題

//list[pointer_interface][ポインタとメソッドセット][go]{
package main

import "fmt"

type Hoge interface {
  F() int
}

type Piyo struct {
  N int
}

func (p *Piyo) F() int {
  p.Inc()
  return p.N
}

func (p Piyo) Inc() {
  p.N++
}

func main() {
  var h Hoge = Piyo{N: 100}
  fmt.Println(h.F(), h.F())
}
//}

@<list>{pointer_interface}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{101 102}と表示される
 3. @<code>{100 100}と表示される
 4. パニックが発生する

=== 解答と解説


問題のコードを実行してみる@<fn>{pointer_interface_pg}と、次のようにコンパイルエラーが発生します。

//footnote[pointer_interface_pg][@<href>{https://play.golang.org/p/xdfeX_IrMOq}]

//cmd{
./main.go:23:6: cannot use Piyo literal (type Piyo) as type Hoge in assignment:
  Piyo does not implement Hoge (F method has pointer receiver)
//}

そのため、答えは1となります。

@<code>{main}関数で定義されている変数@<code>{h}の方は@<code>{Hoge}です。
@<code>{Hoge}はインタフェースであるため、@<code>{h}に代入できるのは@<code>{Hoge}インタフェースを実装した型の値となります。

ある型が@<code>{Hoge}インタフェースを実装するには、その型が@<code>{F() int}というメソッドを持っている必要があります。

@<list>{pointer_interface}では、構造体型の@<code>{Piyo}型が定義されています。
そして、そのポインタ型をレシーバとしてメソッド@<code>{F}が実装されています。

言語仕様@<fn>{methodset_spec}を見ると、@<code>{*T}型のメソッドセットには@<code>{T}のメソッドセットも含みますが、@<code>{T}型のメソッドセットには@<code>{*T}型のメソッドを含みません。
そのため、@<code>{main}関数で変数@<code>{h}に代入しようとしている値は@<code>{Piyo}型の値であるため、コンパイルエラーとなります。

//footnote[methodset_spec][@<href>{https://golang.org/ref/spec#Method_sets}]

@<list>{methodset_example}のようなシンプルな例で見てみると、言語仕様で言わんとしていることが分かるでしょう。

//list[methodset_example][T型と*T型のメソッドセット][go]{
package main

type T struct{}

func (_ T) F() {}

func (_ *T) G() {}

func main() {
  var _ interface{ F() } = &T{}
  var _ interface{ G() } = T{} // こちらはエラーになる
}
//}

なお、@<list>{methodset_example}を実行すると@<fn>{methodset_example_pg}次のような結果になります。

//footnote[methodset_example_pg][@<href>{https://play.golang.org/p/BdsE7GYUwig}]

//cmd{
main.go:11:6: cannot use T literal (type T) 
as type interface { G() } in assignment:
  T does not implement interface { G() } (G method has pointer receiver)
//}

また、@<list>{pointer_interface}の@<code>{var h Hoge = Piyo{N: 100}}を@<code>{var h Hoge = &Piyo{N: 100}}とした場合には、@<code>{Inc}メソッドのレシーバがポインタではないため、@<code>{100 100}と表示されます。

この問題はレシーバをポインタにしないとフィールドへの代入が意味がないことを確かめる問題に見せかけて、ポインタのメソッドセットの仕様を理解しているかをチェックする問題でした。

== マップの初期化

=== 問題

//list[map_init][マップの初期化][go]{
package main

import "fmt"

type Key struct {
  V1 int
  V2 [2]int
}

func main() {
  var m map[Key]int
  key := Key{V1: 100, V2: [2]int{1, 2}}
  m[key] = 100
  fmt.Println(m[key])
}
//}

@<list>{map_init}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{100}と表示される
 3. @<code>{0}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{map_init_pg}と次のようにパニックが発生します。

//footnote[map_init_pg][@<href>{https://play.golang.org/p/5dXZGhxnILM}]

//cmd{
panic: assignment to entry in nil map

goroutine 1 [running]:
main.main()
  /tmp/sandbox467872364/main.go:13 +0x80
//}

変数@<code>{m}はマップで初期化されていないため、
値はゼロ値である@<code>{nil}になっています。
そのため、@<code>{m[key] = 100}のように代入しようとするとパニックが発生してしまいます。

なお、変数@<code>{m}を@<code>{m := map[Key]int{}}のように初期化しておいた場合には、@<code>{100}が表示されます。
マップのキーにできる型は@<code>{==}で比較できる型であれば問題ありません。
そのため、構造体や配列は@<code>{==}で比較可能であるため、マップのキーに用いることができます。
しかし、構造体のフィールドや配列の要素に@<code>{==}で比較できない型が含まれている場合は、その構造体や配列も@<code>{==}で比較できなくなるため、マップのキーに用いることはできません。
@<code>{==}で比較できない型とは、関数やスライスになります。

@<list>{map_init}の@<code>{Key}構造体のフィールド@<code>{V2}は配列であるため、マップのキーとして用いることは問題ありません。しかし、@<code>{V2}がスライスであった場合は、@<code>{Key}型をマップのキーとして利用することはできなくなります。

この問題ではマップのキーに使える型はどういうものなのかという疑問を持ちながら解答することで、うっかりマップの初期化という初歩的なミスを見落としてしまうという点を突いたものです。

== lenと配列のポインタ

=== 問題

//list[len_array_ptr][lenと配列のポインタ][go]{
func main() {
  ns := [...]int{10, 20, 30}
  ptr := &ns
  println(len(ptr))
}
//}

@<list>{len_array_ptr}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{3}と表示される
 3. @<code>{0}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{len_array_ptr_pg}と@<code>{3}が表示されます。

//footnote[len_array_ptr_pg][@<href>{https://play.golang.org/p/0Xy_4aqOU_d}]

組込み関数である@<code>{len}は、次のような型の値を引数として受け付けます@<fn>{spec_len}。

 * スライス
 * マップ
 * チャネル
 * 配列
 * 配列のポインタ

//footnote[spec_len][@<href>{https://golang.org/ref/spec#Length_and_capacity}]

そのため、@<code>{ptr}は配列のポインタであるため、そのポインタが指す配列の要素数となり@<code>{3}が表示されます。

== lenとスライスのポインタ

=== 問題

//list[len_slice_ptr][lenとスライスのポインタ][go]{
func main() {
  ns := []int{10, 20, 30}
  ptr := &ns
  println(len(ptr))
}
//}

@<list>{len_slice_ptr}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{3}と表示される
 3. @<code>{0}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{len_slice_ptr_pg}と次のようなコンパイルエラーが発生します。

//footnote[len_slice_ptr_pg][@<href>{https://play.golang.org/p/ALWAWAtQSCJ}]

//cmd{
main.go:6:13: invalid argument ptr (type *[]int) for len
//}

前述の通り、組み込み関数の@<code>{len}はスライスのポインタを取ることができないため、
コンパイルエラーが発生してしまいます。

== lenとチャネル

=== 問題

//list[len_chan][lenとチャネル][go]{
func main() {
  ch := make(chan int, 10)
  println(len(ch))
}
//}

@<list>{len_chan}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{10}と表示される
 3. @<code>{0}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{len_chan_pg}と@<code>{0}が表示されます。

//footnote[len_chan_pg][@<href>{https://play.golang.org/p/AhQJj-pVNPd}]

組込み関数の@<code>{len}の引数にチャネルを渡した場合、チャネル内に保持されてる値の数が返されます。
そのため、@<list>{len_chan}では1つも値を入れてないため、@<code>{0}が表示されます。

一方、組込み関数の@<code>{cap}はチャネルの容量を返すため、@<list>{len_chan}のチャネルの場合は@<code>{10}が返されます。

この問題は、チャネルに対して組込み関数の@<code>{len}や@<code>{cap}が使えることと、それぞれの挙動のち外を理解しているかを問う問題です。

== closeの複数回呼び出し

=== 問題

//list[close_close][closeの複数回呼び出し][go]{
func main() {
  ch := make(chan int, 10)
  ch <- 100
  close(ch)
  close(ch)
  println(len(ch))
}
//}

@<list>{len_chan}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{10}と表示される
 3. @<code>{1}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{close_close_pg}と次のようにパニックが発生します。

//footnote[close_close_pg][@<href>{https://play.golang.org/p/IhlG2jK7vFA}]

//cmd{
panic: close of closed channel

goroutine 1 [running]:
main.main()
  /tmp/sandbox289574606/main.go:7 +0xa0
//}

クローズされたチャネルに対して再度@<code>{close}を呼び出すとパニックが発生します。

また、@<code>{close}の呼び出しが1度だけであった場合、@<code>{close}されたチャネルであっても@<code>{len}関数を呼び指すとチャネル内に持つデータの数を返すため、@<code>{1}と表示されます。

== closeによるブロードキャスト

=== 問題

//list[chan_close][closeによるブロードキャスト][go]{
func main() {
  ch1 := make(chan int)
  ch2 := make(chan int)
  go func() { <-ch1; print("A"); close(ch2) }()
  close(ch1)
  <-ch2
  print("B")
}
//}

@<list>{chan_close}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{AB}と表示される
 3. @<code>{BA}と表示される
 4. デッドロックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{chan_close_pg}と@<code>{AB}と表示されます。

//footnote[chan_close_pg][@<href>{https://play.golang.org/p/Mkza5ajAg8T}]

チャネルが@<code>{close}されるとそのチャネルから受信を行っている箇所に直ちに@<code>{close}されたことが第1戻り値のゼロ値と第2戻り値の@<code>{false}で伝えられます。

そのため、@<code>{ch1}と@<code>{ch2}を初期化したあとにゴールーチンが生成されます。
その後、@<code>{main}ゴールーチンが@<code>{ch1}を@<code>{close}し、@<code>{<-ch2}で@<code>{ch2}からの受信を待ちます。

その間にもう一つのゴールーチンに処理が切り替わり、@<code>{<-ch1}で@<code>{ch1}からの受信を行います。
しかし、@<code>{ch1}はすでに@<code>{close}されているため、@<code>{<-ch1}は直ちに終了し、次の処理である@<code>{print("A")}が実行されます。

そして、@<code>{ch2}が@<code>{close}され、それが@<code>{main}ゴールーチンの@<code>{<-ch2}を行っている箇所に伝えられます。
そこから次の処理に移り、@<code>{print("B")}が実行されます。
最終的には@<code>{AB}が表示されていることになります。

この問題は特に引っ掛けみたいなものはなく、単純に@<code>{close}の挙動を理解しているかを問う問題です。

== nilチャネル

=== 問題

//list[nil_chan][nilチャネル][go]{
func main() {
  var ch1 chan chan int
  select {
  case <-ch1:
    println("A")
  default:
    println("B")
  }
}
//}

@<list>{nil_chan}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{A}と表示される
 3. @<code>{B}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{nil_chan_pg}と@<code>{B}と表示されます。

//footnote[nil_chan_pg][@<href>{https://play.golang.org/p/dd2DZZ5N2cG}]

@<list>{nil_chan}を見ると、チャネルのチャネルである@<code>{var ch1 chan chan int}に目が行きがちですが、この部分は問題を解く上で重要ではありません。
チャネルはファーストクラスオブジェクトであり、引数や戻り値に取ることができます。
そのため、チャネルのチャネルを定義することができることは、特に不自然ではありません。

一方、チャネル型のゼロ値は@<code>{nil}であり、@<list>{nil_chan}の@<code>{ch1}も@<code>{make}関数で明示的に初期化されているわけではないため、値は@<code>{nil}です。

値が@<code>{nil}であるチャネルを@<code>{select}の@<code>{case}に用いた場合、
その@<code>{case}は絶対に実行されることはありません。
これをうまく使い、@<code>{nil}を代入することで特定のチャネルからの受信を一時的に止めることに使うようなパターンを@<code>{nil}チャネルパターンと呼びます。

== 型エイリアス

=== 問題

//list[type_alias][型エイリアス][go]{
func main() {
  type A = int
  var a A
  fmt.Printf("%T", a)
}
//}

@<list>{type_alias}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{A}と表示される
 3. @<code>{int}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{type_alias_pg}と@<code>{int}と表示されます。

//footnote[type_alias_pg][@<href>{https://play.golang.org/p/NPL652NE-14}]

@<code>{type A = int}のように記述すると型@<code>{int}のエイリアスとして、型@<code>{A}を定義するという意味になり、この機能を型エイリアスと呼びます。
型エイリアスは、@<code>{type A int}のように新しい型として定義しているわけではなく、まったく同じ型としてエイリアスを作成しているだけです。

そのため、@<code>{fmt.Printf}の@<code>{%T}書式で型名を表示させると、エイリアス元である@<code>{int}が表示されます。

なお、@<code>{type A = int}ではなく、@<code>{type A int}のように定義してあった場合は、@<code>{A}が出力されます。

この問題はGo1.9で導入された型エイリアスがどのようなものか理解していることが問われる問題です。

== 変数true

=== 問題

//list[var_true][変数true][go]{
func main() {
  true := false
  println(true == false)
}
//}

@<list>{var_true}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{true}と表示される
 3. @<code>{false}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{var_true_pg}と@<code>{true}と表示されます。

//footnote[var_true_pg][@<href>{https://play.golang.org/p/dd2DZZ5N2cG}]

@<code>{true}や@<code>{false}は予約語ではありません。
そのため、@<code>{true}や@<code>{false}といった名前の変数を作ることが可能です。

@<list>{var_true}では@<code>{true}という名前の変数を作り、そこに値として@<code>{false}を代入しています。
そして、@<code>{println(true == false)}は、変数である@<code>{true}と値の@<code>{false}を比較しており、変数@<code>{true}には@<code>{false}が入っているため、@<code>{false == false}となり、@<code>{true}が表示されます。

この問題は、予約語とはなにか、予約語にはどのようなものがあるかなどを理解しているかを問う問題です。

== 大きな定数の演算

=== 問題

//list[const][大きな定数の演算][go]{
func main() {
  n := 1000000000000000000 / 1000000000000000000
  println(n)
}
//}

@<list>{const}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{1}と表示される
 3. @<code>{NaN}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{const_pg}と@<code>{1}と表示されます。

//footnote[const_pg][@<href>{https://play.golang.org/p/pHO_-1Mkfu2}]

定数は明示的に型を指定しない場合、特定の型を持ちません@<fn>{goblog_const}。
そのため、十分に大きいな定数であっても@<code>{int}の範囲に収まっていなくても、型なしの定数同士の演算であれば問題ありません。

//footnote[goblog_const][@<href>{https://blog.golang.org/constants}]

演算の結果を変数に代入するときになってはじめて、その変数の型のサイズに収まるかが問題になってきます。
@<list>{const}で用いられている定数の@<code>{1000000000000000000}は十分に@<code>{int}型の範囲を超えています。
しかし、@<code>{1000000000000000000 / 1000000000000000000}の結果は@<code>{1}となり、@<code>{println}に渡されtる段階では@<code>{int}型の範囲に収まっているため、特にコンパイルエラーは発生せずに実行ができます。

この問題はGoの定数を正しく理解しているかを問われる問題です。

== nilなレシーバ

=== 問題

//list[nil_receiver][nilなレシーバ][go]{
type Hoge struct{N int}

func (h *Hoge) F() {
  fmt.Println(h)
}

func main() {
  var h *Hoge
  h.F()
}
//}

@<list>{nil_receiver}を実行した結果は次のうちどれになるでしょうか？

 1. コンパイルエラー
 2. @<code>{{0}}と表示される
 3. @<code>{<nil>}と表示される
 4. パニックが発生する

=== 解答と解説

問題のコードを実行してみる@<fn>{nil_receiver_pg}と@<code>{<nil>}と表示されます。

//footnote[nil_receiver_pg][@<href>{https://play.golang.org/p/1GfQZ3QWAXi}]

メソッドのレシーバはほとんど引数と同じ扱いを受けます。
そのため、レシーバはメソッド呼び出し毎にコピーされてメソッドに渡されます。
また、レシーバの値が@<code>{nil}であっても呼び出すことができます。

この問題はレシーバがどういう扱いを受けるのか理解していることを問う問題です。

== おわりに

この章ではGoの文法や言語仕様、ランタイムの挙動の知識が問われるGoクイズについて扱いました。
ここで扱ったコードはほとんどがプロダクトコードで書くようなコードでは無いうえに、コンパイラが指摘したりテストで気づけるものが多かったでしょう。

しかし、言語を深く理解することは悪いことではなく、効率よくコードを書く支えになってくれます。
この章を通じて言語仕様やランタイムの挙動に興味を持ち、コンパイラやランタイムのコード読む方が増えてくれると嬉しいです。
