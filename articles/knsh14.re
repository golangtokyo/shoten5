= go testのオプション大全
Goではテストツールとして@<code>{go test}というコマンドが標準ツールとして提供されています。

本章では@<code>{go test}で使うことのできるフラグの役割を解説し、テストにどのような影響を与えるか解説します。
なお、対象にするGoのバージョンは2018年10月1日時点で最新の1.11とします。

== go testのフラグ一覧
@<tt>{go test}のヘルプを見るためには@<code>{go test -h}で見ることができます。
ソースコードでゆっくり見たい場合にはGitHubにコード@<fn>{knsh14_link_gotestsourcecode}があるので最新のものを見ることができます。

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

それぞれのフラグについて解説していきます。

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

これを実行する際に@<code>{go test -v -args -num 3}と実行すると@<list>{knsh14_result_arg}のような出力になります。

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

=== c
このフラグをつけて@<tt>{go test}を実行すると、テスト関数を実行するのではなく、テストを実行するバイナリを生成します。
バイナリ名は何も指定しないと@<tt>{パッケージ名.test}になります。バイナリの名前は後述する@<tt>{-o}オプションで指定することもできます。
@<tt>{go build}のテスト版だと思っていただけるとイメージが付きやすいです。

このフラグが有効な場合、複数パッケージを同時に扱うことはできません。
生成されたバイナリはカテゴリ2のフラグを利用することができます。

ただし、バイナリにカテゴリ2のフラグを渡す際にはそのまま渡すことができません。
@<tt>{-run}オプションなら@<tt>{-test.run}と変更する必要があります。

=== exec xprog
@<tt>{go test}で実行されるバイナリ対して外部コマンドを実行します。
例を挙げると@<code>{go test -exec "go tool test2json"}のように使います。

@<tt>{-c}や@<tt>{-o}フラグと同時に使うことはできません。

=== i

=== json
標準出力に出される結果をJSON形式にしてテストを実行します。
この際に、カテゴリ2の@<tt>{-v}オプションも同時に付与され、詳細なログが出力されます。
JSONの出力形式についてより詳しく知りたい場合には@<code>{go doc test2json}からドキュメントを見ることができます。@<fn>{knsh14_link_doctotest2json}

//footnote[knsh14_link_doctotest2json][@<href>{https://golang.org/cmd/test2json/}]

詳細なログはいらないという場合には@<tt>{-exec}フラグを使って@<code>{go test -exec "go tool test2json"}とします。

=== o
テストバイナリを渡された名前で生成します。
これだけ聞くと@<tt>{-c}と大きな差がないように見えます。
しかし、このオプションだけをつけて実行した場合はテストが実行された上でバイナリが生成されます。