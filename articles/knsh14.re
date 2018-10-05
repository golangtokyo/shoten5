= go testのマニアックなオプション大全
メルペイのバックエンドエンジニアの@knsh14@<fn>{knsh14_link_twitter}です。
//footnote[knsh14_link_twitter][@<href>{https://twitter.com/knsh14}]

Goではテストツールとして@<code>{go test}というコマンドが標準ツールとして提供されています。
@<tt>{go test}はテストを実行して動作を検証するだけでなく、ベンチマークを計測したり、ドキュメント用にExampleを作成したりすることができます。

本章では@<code>{go test}で使うことのできるフラグの役割を解説し、実行にどのような影響を与えるか解説します。
特に普段使う場面の少ないであろうフラグに絞って説明をしたいと思います。
testingパッケージの使い方などの解説はしません。そちらについては公式のドキュメントや書籍などを参照してください。
なお、対象にするGoのバージョンは2018年10月1日時点で最新の1.11とします。


== go testのフラグ一覧
@<tt>{go test}で使うことのできるフラグを見るためには@<code>{go test -h}を使います。
ブラウザで見たい場合にはGitHubに@<tt>{go test -h}で出力されるものと同じもの@<fn>{knsh14_link_gotestsourcecode}があるので最新のものを見ることができます。

//footnote[knsh14_link_gotestsourcecode][@<href>{https://github.com/golang/go/blob/release-branch.go1.11/src/cmd/go/internal/test/test.go}]

@<tt>{go test}のフラグにはいくつか種類があります。

 1. @<tt>{-json}などの@<tt>{go test}そのものによって解釈されるもの
 2. @<tt>{-run}や@<tt>{-v}など@<tt>{go test}によって実行されるテストが影響をうけるもの

これらは@<tt>{go test -h}で見た場合には特に区別されていませんが、ソースコードを読んでみるとそれぞれ別の変数で定義されていることがわかります。@<fn>{knsh14_gotestFlag1}@<fn>{knsh14_gotestFlag2}
本章では便宜上カテゴリ1、カテゴリ2という呼び方で両者を区別します。

//footnote[knsh14_gotestFlag1][@<href>{https://github.com/golang/go/blob/release-branch.go1.11/src/cmd/go/internal/test/test.go#L136}]
//footnote[knsh14_gotestFlag2][@<href>{https://github.com/golang/go/blob/release-branch.go1.11/src/cmd/go/internal/test/test.go#L197}]

== カテゴリ1 go testそのものによって解釈されるもの
この種類に分類されるフラグは以下のものがあります。

 * @<tt>{-args}
 * @<tt>{-c}
 * @<tt>{-exec xprog}
 * @<tt>{-i}
 * @<tt>{-json}
 * @<tt>{-o file}

=== args
@<tt>{go test}自体にフラグを渡すのではなく実行するテストコードにフラグを渡すことができます。
@<tt>{-args}以降のフラグは全て渡されます。

少し無理やりですが@<list>{knsh14_code_arg}は@<tt>{args}フラグを使うことを前提としたコードです。
@<tt>{flag}パッケージで引数を処理し、@<code>{IsMultiple}関数の動作を変更できるようになっています。

//list[knsh14_code_arg][コマンドライン引数を取るテストコード]{
package main

import (
  "flag"
  "testing"
)

var (
  num int
)

func init() {
  flag.IntVar(&num, "num", 1, "")
}

func IsMultiple(n, m int) bool {
  return n%m == 0
}

func TestSampleOdd(t *testing.T) {
  flag.Parse()
  cases := []struct {
    input int
  }{
    {input: 1},
    {input: 3},
    {input: 5},
    {input: 7},
  }
  for _, c := range cases {
    if IsMultiple(c.input, num) {
      t.Log(c.input)
    } else {
      t.Errorf("%d is not multiple of %d", c.input, num)
    }
  }
}
//}

@<list>{knsh14_code_arg}を実行する際に@<code>{go test -v -args -num 3}と実行すると@<list>{knsh14_result_arg}のような出力になります。

//list[knsh14_result_arg][実行結果]{
knsh14% go test -v -args -num 3
=== RUN   TestSampleOdd
--- PASS: TestSampleOdd (0.00s)
    sample_test.go:34: 1 is not multiple of 3
    sample_test.go:32: 3
    sample_test.go:34: 5 is not multiple of 3
    sample_test.go:34: 7 is not multiple of 3
PASS
ok      github.com/knsh14/sample        0.007s
//}

実践での使い所としては例のように、実行時に挙動を変えたいなどがありそうです。
DBの接続先や、タイムアウトの調整などで役に立ちそうです。

=== c
このフラグをつけて@<tt>{go test}を実行すると、テスト関数を実行するのではなく、テストを実行するバイナリを生成します。
バイナリ名は何も指定しないと@<tt>{パッケージ名.test}になります。バイナリの名前は後述する@<tt>{-o}オプションで指定することもできます。
@<tt>{go build}のテスト版だと考えるとイメージが付きやすいです。

このフラグが有効な場合、複数パッケージを同時に扱うことはできません。
生成されたバイナリはカテゴリ2のフラグを利用することができます。

ただし、バイナリにカテゴリ2のフラグを渡す際にはそのまま渡すことができません。
@<tt>{-run}オプションなら@<tt>{-test.run}と変更する必要があります。

使い所としてはテストを作成して実行する前に使います。
事前にコンパイルしておいて最低限build failedで失敗しないように対策することで安心して実行することができます。

=== exec xprog
@<tt>{go test}で実行されるバイナリ対して外部コマンドを実行します。
例を挙げると@<code>{go test -exec "go tool test2json"}のように使います。

@<tt>{-c}や@<tt>{-o}フラグと同時に使うことはできません。

使い所がなんとも難しいフラグです。
@<tt>{-c}フラグと使えるとハッシュが取得できてありがたいなどがあるかもしれません。
しかしそれもできないので、現状だと@<code>{go test -exec "go tool test2json"}くらいしか使いみちがなさそうに見えます。

=== i
ドキュメントを見ると「テストが依存しているパッケージをインストールします。テストは実行されません。」とあります。
が実際には何がされているか全くわかりません。

=== json
標準出力に出される結果をJSON形式にしてテストを実行します。
この際に、カテゴリ2の@<tt>{-v}オプションも同時に付与され、詳細なログが出力されます。
JSONの出力形式についてより詳しく知りたい場合には@<code>{go doc test2json}からドキュメントを見ることができます。@<fn>{knsh14_link_doctotest2json}

詳細なログはいらないという場合には@<tt>{-exec}フラグを使って@<code>{go test -exec "go tool test2json"}とします。

CIなどでテストを実行した結果を@<code>{jq}などに食わせて処理をしたい場合に便利かもしれません。

//footnote[knsh14_link_doctotest2json][@<href>{https://golang.org/cmd/test2json/}]

=== o
テストバイナリを渡された名前で生成します。
これだけ聞くと@<tt>{-c}と大きな差がないように見えます。
しかし、このオプションだけをつけて実行した場合はテストが実行された上でバイナリが生成されます。

このフラグだけで使うよりも、@<tt>{-c}オプションと組み合わせて使う場面のほうが多そうに見えます。

== カテゴリ2  go testによって実行されるテストが影響をうけるもの
このカテゴリに分類されるフラグは以下のものがあります。

 * @<tt>{-bench regexp}
 * @<tt>{-benchtime t}
 * @<tt>{-count n}
 * @<tt>{-cover}
 * @<tt>{-covermode set,count,atomic}
 * @<tt>{-coverpkg pattern1,pattern2,pattern3}
 * @<tt>{-cpu 1,2,4}
 * @<tt>{-failfast}
 * @<tt>{-list regexp}
 * @<tt>{-parallel n}
 * @<tt>{-run regexp}
 * @<tt>{-short}
 * @<tt>{-timeout d}
 * @<tt>{-v}
 * @<tt>{-vet list}

こちらに分類されるフラグは使っている人も多いのではないでしょうか。

さらに、プロファイル用のフラグとして、以下のフラグが用意されています。

 * @<tt>{-benchmem}
 * @<tt>{-blockprofile block.out}
 * @<tt>{-blockprofilerate n}
 * @<tt>{-coverprofile cover.out}
 * @<tt>{-cpuprofile cpu.out}
 * @<tt>{-memprofile mem.out}
 * @<tt>{-memprofilerate n}
 * @<tt>{-mutexprofile mutex.out}
 * @<tt>{-mutexprofilefraction n}
 * @<tt>{-outputdir directory}
 * @<tt>{-trace trace.out}

この中から以下のフラグについて解説しようと思います。

 * @<tt>{-failfast}
 * @<tt>{-timeout d}

=== failfast
このフラグを付けて実行した場合、テストが最初に失敗した段階で残りの項目を全てスキップします。
テスト関数内で@<tt>{testing.T.Run}関数でサブテストを実行している場合も、サブテストの一つが失敗した段階で終了します。

PR を出す前などに最終チェックをする場合や、CIで実行時間を短くしたい場合などに非常に便利です。

=== timeout
@<tt>{go test}の実行自体のタイムアウトを設定します。
これを過ぎた場合、panic を起こしてテストを強制終了します。

フラグを付けた時点でタイムアウトが有効になります。
デフォルトは10分で設定されています。
フラグを付けた場合0に設定するとタイムアウトを無効にできます。

== 最後に
本章では普段なかなか使わない@<tt>{go test}のフラグを解説しました。
普段使わないだけあって使い所に困るものもありましたが、調べているうちに便利なものも発見できました。

テストツールに何ができるか知っておくと、「ここはテストツールの機能でカバーしよう」といったより柔軟な選択肢を取ることができます。
これをきっかけに他のツールも調べてみようと思っていただければ幸いです。

