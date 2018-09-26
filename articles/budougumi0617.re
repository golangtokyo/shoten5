={budougumi0617_title} go-cloudとWireコマンドの概要と活用方法

== はじめに
freee株式会社@<fn>{freee}でバックエンドエンジニアをしておりますbudougumi0617@<fn>{budougumi0617_fn1}です。
普段は主にGoとgRPCによるマイクロサービスを開発しています。

//footnote[freee][@<href>{https://corp.freee.co.jp}]
//footnote[budougumi0617_fn1][@<href>{https://twitter.com/budougumi0617}]

本章では7月に発表されたgo-cloud@<fn>{github_gocloud}とそれに付属するWireコマンドについて次の情報をまとめています。

 * go-cloudの基本的な概要
 * Wireの基本的な概要
 * Wireを使ったDependency Inejection（依存性注入）の利用方法
 * Wireを使ったDependency Inejection（依存性注入）の提供方法

//footnote[github_gocloud][@<href>{https://github.com/google/go-cloud}]

=== 注意
本稿は2018/09/23時点のgo-cloudリポジトリを参考に記述されています。
go-cloudは現時点でAlphaリリース(v0.2.0)であるため、今後本稿の情報には差異が生じる可能性があります。
また、本章で引用しているgo-cloudのコード、記載しているコードは全てApache License 2.0です。

== go-cloudの概要
go-cloudはGoogleが公開したポータブルなクラウドアプリケーションを開発するためのGo用のAPIライブラリです。
2018/07/24にGo blogで"Portable Cloud Programming with Go Cloud"@<fn>{goblog}というタイトルでリリースアナウンスされました。

//footnote[goblog][@<href>{https://blog.golang.org/go-cloud}]

2018/09/23時点でGitHubに公開されているgo-cloudのリポジトリには次の内容が含まれています。

 * BLOBストレージのジェネリックなAPI定義
 * GCP, AWSのBLOBストレージサービス、MySQLサービスをラップした実装
 * ロギングやヘルスチェックなどの基本的なWebサーバの構成に必要な実装
 * Dependency Injection@<fn>{dependency_injection}を行なうためのWireコマンド

//footnote[dependency_injection][@<href>{https://en.wikipedia.org/wiki/Dependency_injection}]

ジェネリックなAPI定義とは、誤解を恐れずにいうとMySQL, postgreSQLなどに対する@<code>{database/sql}パッケージに相当するようなものです。
また、より簡単にクラウドサービスへの依存関係をコードで表現できるように、Wireコマンドを使ったDependency Injection(DI、依存性の注入)の仕組みが提供されています。

=== go-cloudの対応クラウドプロバイダー
各クラウドプロパイダー用の実装は2018/09/23時点でGoogle Cloud Platform(GCP)、Amazon Web Services(AWS)のみです。
クラウドプロパイダーではありませんが、ローカル環境へのアクセスを想定した実装@<fn>{local_impl}は同梱されています。
GCP、AWS別ベンダーのサポート計画などはとくに公表されていません。
Azureのサポートについてのissue@<fn>{azure_issue}などは立ち上がっています。

//footnote[local_impl][@<href>{https://github.com/google/go-cloud/tree/master/samples/guestbook/localdb}]
//footnote[azure_issue][@<href>{https://github.com/google/go-cloud/issues/76}]

== go-cloudがなぜ必要なのか？
クラウドプロパイダーを利用したWebアプリケーション開発が一般的になった今日ですが、kubernetes(k8s)やDockerを利用したコンテナ技術も身近になってきました。
コンテナ技術を活用することでWebアプリケーションのポータビリティ性は高まっていると言えます。
しかし、実際にWebアプリケーションは本当にマルチクラウドで利用できるようになっているのでしょうか？
AWS上で動いているコンテナのアプリケーションロジックはそのままGCP上で可動できるのでしょうか？
多くの場合、アプリケーションはクラウドプロパイダーのSDKやマネージド・サービスのAPIに大きく依存しており、アプリをk8sに載せたとしてもベンダー依存からは脱却できていません。
クラウドプロパイダーを乗り換える、あるいはマルチクラウド対応を行う際はベンダーサービスへの依存部分を再実装する必要があります。
このような現状で、go-cloudはGoのアプリケーションにクラウドサービスへのジェネリックなAPIを定義し、クラウドプロバイダーに依存しない開発を行うために公開されました。


例えば、go-cloudに含まれているsamples/tutorial/main.go(@<list>{tutorial_main})を見てみましょう。
このサンプルは指定されたファイル名（@<code>{file}）のファイルをコマンドオプションで指定されたAWS/GCPいずれかのBLOBストレージにアップロードするコードです。

//list[tutorial_main][go-cloud/samples/tutorial/main.goから抜粋したファイルアップロード処理]{
ctx := context.Background()
// Open a connection to the bucket.
b, err := setupBucket(context.Background(), *cloud, *bucketName)

// Prepare the file for upload.
data, _ := ioutil.ReadFile(file)

w, _ := b.NewWriter(ctx, file, nil)
w.Write(data)
w.Close()
//}

@<code>{go-cloud/blob}パッケージに含まれる@<code>{blob.Bucket}のAPI定義に基づいてファイルアップロードのロジックが書かれています。
@<code>{setupBucket()}メソッド内で@<code>{go-cloud/blob/s3blob}と@<code>{go-cloud/blob/gcsblob}のどちらかを生成する処理がありますが、
main.goの実装にクラウドプロパイダーに依存した処理はありません。
@<code>{blob.Bucket}オブジェクトの初期化処理を隠蔽すれば、コーディングを行う開発者はクラウドプロパイダーのSDKを考慮せずに開発することができます。
それでは、次節からgo-cloudリポジトリの中を確認していきます。

== go-cloudリポジトリの内容
go-cloudリポジトリには大きく分けて@<table>{repo_content}の内容が含まれています。

//table[repo_content][go-cloudリポジトリの内容]{
分類	内容
-------------------------------------------------------------
メインコード	ジェネリックなAPI定義とAWS/GCPに対応した実装
@<code>{wire}パッケージ	Wireコマンドを利用したDIの仕組み
@<code>{samples}パッケージ	サンプルコード
//}

go-cloudのリポジトリには大まかにジェネリックなAPI定義が含まれています。
また、それぞれの実装は@<hd>{wire_section}から説明する@<code>{go-cloud/wire}パッケージとWireコマンドを利用したDIを行うための設計がされています。
サンプルは3種類あり、それぞれの概要は@<table>{samples_content}のとおりです。

//table[samples_content][go-cloud/samplesに用意されているコンテンツ内容]{
サンプル名	内容
-------------------------------------------------------------
tutorial	@<code>{go-cloud/blob}パッケージを使ったCLIツール
wire	Wireコマンドを使ったDIのサンプル
guestbook	AWS/GCP上で実行できるWireを駆使したWebサーバのサンプル
//}

ではgo-cloudで定義されたジェネリックなAPIを確認していきましょう。
といっても、2018/09/23現在、go-cloudで定義されたクラウドサービスのジェネリックなAPI定義はBLOBストレージ用しかありません@<fn>{wrong_sql}。

//footnote[wrong_sql][AWSのRDS, GCP Cloud SQLのラッパーもありますが、go-cloud独自のAPIではなく@<code>{database/sql.DB}オブジェクトを生成するビルド関数です。]


== go-cloud/blobパッケージ
@<code>{go-cloud/blob}パッケージはジェネリックなAPIとして@<code>{blob.Bucket}を提供しています。
@<code>{go-cloud/blob}パッケージのサブパッケージにある@<code>{OpenBucket}関数を用いると、各クラウドプロパイダー用の設定がされた@<code>{blob.Bucket}オブジェクトを生成することが出来ます。

//list[s3blob_open_bucket][go-cloud/blob/s3blob/s3blob.go]{
func OpenBucket(ctx context.Context,
                sess client.ConfigProvider,
                bucketName string
                ) (*blob.Bucket, error)
//}

//list[gcsblob_open_bucket][go-cloud/blob/gcsblob/gcsblob.go]{
func OpenBucket(ctx context.Context,
                bucketName string,
                client *gcp.HTTPClient
               ) (*blob.Bucket, error)
//}

== go-cloud/mysql/{cloudmysql, rdsmysql}パッケージ
go-cloud v0.2.0時点ではBLOBストレージへのAPI定義以外に独自定義されたAPI定義はありません。
ただし、@<hd>{wire_section}で説明するWireコマンドによるDIを考慮した設計のSQLサービスの実装は用意されています。

//list[cloudmysql][go-cloud/mysql/cloudmysql.Open関数]{
// Open opens a Cloud SQL database.
func Open(ctx context.Context, certSource proxy.CertSource, params *Params) (*sql.DB, error)
//}

//list[rdsmysql][mysql/rdsmysql/rdsmysql.Open関数]{
// Open opens an encrypted connection to an RDS MySQL database.
func Open(ctx context.Context, provider CertPoolProvider, params *Params) (*sql.DB, func(), error)
//}

その他に、Stackdriverへのコネクタなどが用意されています。こちらもWireコマンドを使ったDIの仕組みの上に実装されています。

== go-cloudの何がよいのか？
と、ここまでgo-cloudの概要をざっくり紹介してきましたが、「別に外部サービスに依存している部分のパッケージ外出しや抽象化はすでにしてあるし」という方も多いと思います。
先に断っておくと、現時点のgo-cloudを利用するだけでバイナリからベンダー依存のコードを完全に削除できるわけではありません@<fn>{no_delete}。
まだ全てのAPI定義が揃っているわけでもありません。
//footnote[no_delete][go-cloudパッケージが内部で参照している]
では、今go-cloudを利用するメリットはなんでしょうか？
デファクトになりえるgoogleによって定義されたクラウドサービスのAPIを使う、という意味以外にもgo-cloudを使う意味はあります。
それはgo-cloudリポジトリにあるWireコマンドの存在と、Wireコマンドによって実現される依存関係の自動生成です。

=={wire_section} Wireの概要
Wireコマンドはgo-cloudリポジトリに付属されているコマンドラインツールです。
Wireコマンドは@<code>{go-cloud/wire}パッケージをつかって定義されたDependency Injection(DI、依存性の注入)を解決するコードを自動生成します。
go-cloud配下のパッケージは@<code>{go-cloud/wire}パッケージを使って依存関係を定義しています。

Wireを使った実装を行うと、私たちは以下の恩恵を受けることができます。

 * DIコードを自動生成できる
 * 依存性を排除したコードを強制できる
 * 複雑な依存関係もWireが自動的に判別してくれる
 * Wireを使って定義された依存関係を組み合わせて、新しい依存関係も定義できる

コンポーネント間の依存関係は、関数のパラメータとしてWireに表示され、グローバル変数の代わりに明示的な初期化を促します。
Wireは実行時の状態やリフレクションなしで動作するため、Wireで使用するように記述されたコードは手書きの初期化にも役立ちます。
各クラウドプロパイダーはBLOBストレージやVMインスタンスという類似サービスを提供していますが、SDKでそれらを利用する際に必要になる情報は異なります。
それとは別に初期化時にクラウドへの接続情報やロギング設定を引き回したいというニーズもあります。大抵は毎回初期化を行うか、グローバル変数に置くことが多いのではないでしょうか。
これらの効率的に解決するために、go-cloudには独自のDIツールであるWireコマンド(パッケージ)@<fn>{wire}が同梱されています。

//footnote[wire][@<href>{https://github.com/google/go-cloud/blob/master/wire/README.md}]

https://github.com/google/go-cloud/tree/master/wire

後述するProviderとInjectorを利用した実装を行なうことで、依存関係を解決するコードをWireコマンドで自動生成することが可能になります。
ちなみに@<code>{go-cloud/wire}パッケージ・Wireコマンドはgo-cloudの他パッケージと独立しているので、WireによるDIの仕組みをまったく別のパッケージで活用することも可能です。

==={pro_and_inj} ProviderとInjector
@<code>{go-cloud/wire}パッケージが提供するDIの仕組みには@<b>{Provider}と@<b>{Injector}という概念があります。
依存関係を定義するProviderと依存関係を注入するInjectorです。


Providerは依存関係の解決が必要なコンポーネントの集合(@<code>{wire.ProviderSet})です。@<code>{wire.Set}関数を使って宣言します。
@<list>{provider}はGCPの依存解決を行うProviderです。

//list[provider][go-cloud/gcp/gcpcloud/gcpcloud.goに定義されているProvider]{
var GCP = wire.NewSet(Services, gcp.DefaultIdentity)

var Services = wire.NewSet(
  cloudmysql.CertSourceSet,
  cloudmysql.Open,
  gcp.DefaultTransport,
  gcp.NewHTTPClient,
  runtimeconfigurator.Set,
  sdserver.Set)
//}

@<code>{Services}はCloud SQL用の認証情報（@<code>{CertSourceSet}）、Cloud SQLへの接続（@<code>{Open}）、
Stack Driverに向けたLogger（@<code>{sdserver.Set}）の依存関係が定義されてるProviderです。
@<code>{GCP}は@<code>{Services}の依存関係に加え、デフォルトのProject IDなどを取得する@<code>{gcp.DefaultIdentity}も含めた依存関係のProviderです。


InjectorはProviderを組み合わせて依存関係を注入するためのコードです。@<code>{wire.Build}関数を使って宣言します。
@<list>{injector_gcp}はProviderで紹介した依存関係@<code>{GCP}などを組み合わせて依存関係を注入した@<code>{*application}オブジェクトを返すInjectorの宣言です。
WireコマンドはこのInjectorｍの宣言からDIを行うコードを自動生成します。Injectorの宣言自体は自動生成を行うためだけのものなので、
ビルドタグ@<fn>{build_tag}をつけてビルドには含めないようにします。

//footnote[build_tag][@<href>{https://golang.org/pkg/go/build/#hdr-Build_Constraints}]

//list[injector_gcp][go-cloud/samples/guestbook/inject_gcp.goにあるInjector]{
//+build wireinject

// setupGCP is a Wire injector function that sets up the application using GCP.
func setupGCP(ctx context.Context,
              flags *cliFlags) (*application, func(), error) {
  // This will be filled in by Wire with providers from the provider sets in
  // wire.Build.
  wire.Build(
    gcpcloud.GCP,
    applicationSet,
    gcpBucket,
    gcpMOTDVar,
    gcpSQLParams,
  )
  return nil, nil, nil
}
//}

上記の例だと自動生成後のコードが膨大になるので、簡単なInejctorを見ます。
@<list>{injector}は３つのビルド関数と外部から与えられる文字列（@<code>{phrase}）を使って依存関係を解決した@<code>{Event}オブジェクトを返すInjector@<fn>{new_events}です。
//footnote[new_events][@<code>{NewEvent}などの宣言はgo-cloud/samples/wire/main.goにあります。]

//list[injector][go-cloud/samples/wire/wire.goにあるInjector]{
//+build wireinject

func InitializeEvent(phrase string) (Event, error) {
  wire.Build(NewEvent, NewGreeter, NewMessage)
  return Event{}, nil
}
//}

実際に依存性を解決するコードはwire genコマンドで自動生成します。
@<list>{injector}からWireコマンドが自動生成したコードが@<list>{wire_gen}です。

//list[wire_gen][go-cloud/samples/wire/wire_gen.go]{
//go:generate wire
//+build !wireinject

func InitializeEvent(phrase string) (Event, error) {
  message := NewMessage(phrase)
  greeter := NewGreeter(message)
  event, err := NewEvent(greeter)
  if err != nil {
    return Event{}, err
  }
  return event, nil
}
//}

どの関数をどの順番で呼び、どの関数の引数に使えば戻り値のオブジェクトの初期化が完了するか、などを自動で判別してコードを生成してくれます。
Wireを使うと、開発者は依存性の解決に必要な情報を書くだけで依存性が注入された初期化済みオブジェクトを手に入れることができます。
では、次節よりWireコマンドの使い方を確認し、ProviderやInjectorの設計方法を確認していきます。

=={how_to_wire} Wireコマンドの使い方
まずgo-cloudリポジトリのコードを使ってWireコマンドの使い方を確認してみましょう。
Wireコマンドは@<code>{go get}コマンドで取得できます。

//cmd{
$ go get github.com/google/go-cloud/wire/cmd/wire
//}

Wireコマンドには3個のコマンドラインオプションがあります。

//cmd{
$ wire -h
usage: wire [gen] [PKG] | wire show [...] | wire check [...]
//}

=== wire gen
wire_gen.goファイルを生成します。引数なしでWireコマンドを実行した際もwire genコマンドが実行されます。


==={wire_show} wire show
定義済みのProviderの使い方を調べる手段として、wire showコマンドが用意されています。
wire showコマンドは引数に指定されたパッケージ内から以下の情報を表示します。

 * パッケージ内のProviderの情報を出力する
 ** Providerが依存するProviderを出力
 ** 必要なInputとそのInputを利用して得られるOutputを知ることができる。
 ** 取得できるOutputを生成しているコードの場所も知ることができる
 * Injectorの情報を出力する

実際にgo-cloudリポジトリのsamples/guestbookでweire showコマンドを実行してみましょう。
@<code>{samples/guestbook}パッケージには@<list>{application_set}のようなapplicationSetがProviderとして用意されています。
また、inject_{aws, gcp, local}.goファイルには複数のInjectorが宣言されています。

//list[application_set][go-cloud/samples/guestbook/main.go]{
// applicationSet is the Wire provider set for the Guestbook application that
// does not depend on the underlying platform.
var applicationSet = wire.NewSet(
    newApplication,
    appHealthChecks,
    trace.AlwaysSample,
)
//}

これだけ見てもこのProviderでどんなことができるかわかりませんね。
@<code>{samples/guestbook}パッケージに対してwire showコマンドを実行した結果が以下になります。

//cmd{
$ pwd
/Users/budougumi0617/go/src/github.com/google/go-cloud/samples/guestbook
$ wire show
"github.com/google/go-cloud/samples/guestbook".applicationSet
Outputs given no inputs:
  go.opencensus.io/trace.Sampler
    at /Users/budougumi0617/go/src/go.opencensus.io/trace/sampling.go:64:6
Outputs given *database/sql.DB:
  []github.com/google/go-cloud/health.Checker
    at /Users/budougumi0617/go/src/github.com/google/go-cloud/samples/guestbook/main.go:313:6
Outputs given *database/sql.DB, *github.com/google/go-cloud/blob.Bucket, *github.com/google/go-cloud/runtimevar.Variable, *github.com/google/go-cloud/server.Server:
  *github.com/google/go-cloud/samples/guestbook.application
    at /Users/budougumi0617/go/src/github.com/google/go-cloud/samples/guestbook/main.go:135:6
Injectors:
  "github.com/google/go-cloud/samples/guestbook".setupAWS
  "github.com/google/go-cloud/samples/guestbook".setupGCP
  "github.com/google/go-cloud/samples/guestbook".setupLocal
//}

wire showコマンドの出力は以下のフォーマットになっています。

//list[show_format][wire showの出力フォーマット]{
{{ 定義されているProvider 1}}
  {{ 依存しているProviderの一覧 }}
Outputs given {{ 必要な入力パターン1 }}
  {{ 必要な入力パターン1があったときProvideSetから得られるオブジェクト一覧1 }}
Outputs given {{ 必要な入力パターン2 }}
  {{ 必要な入力パターン2があったときProvideSetから得られるオブジェクト一覧2 }}
...
{{ 定義されているProvider 2}}
  {{ 依存しているProviderの一覧 }}
...
Injectors:
  {{ 定義されているInjectorの一覧 }}
//}


まず、@<list>{show_format}では表示されていませんが、wire showコマンドで表示するProviderが他のProviderに依存した定義になっている場合、依存しているProviderの一覧も表示されます。
次のコマンドライン出力の結果は@<code>{go-cloud/gcp/gcpcloud}パッケージに対してwire showコマンドを実行した結果です。
GCPというProviderから得られるOutputを出力する前に、GCPが依存するProviderの一覧が表示されています。


//cmd{
$ wire show
"github.com/google/go-cloud/gcp/gcpcloud".GCP
  "github.com/google/go-cloud/gcp".DefaultIdentity
  "github.com/google/go-cloud/gcp/gcpcloud".Services
  "github.com/google/go-cloud/mysql/cloudmysql".CertSourceSet
  "github.com/google/go-cloud/runtimevar/runtimeconfigurator".Set
  "github.com/google/go-cloud/server".Set
  "github.com/google/go-cloud/server/sdserver".Set
Outputs given no inputs:
  ...
//}

次に何をインジェクトすればどんなオブジェクトが取得できるのか、が確認できます。またその提供元の実装コードの場所の情報も出力されます。
例えば、以下の情報は@<code>{*sql.DB}をインジェクトできれば、@<code>{health.Checker}の配列が得られることを示しています。

//cmd{
Outputs given *database/sql.DB:
  []github.com/google/go-cloud/health.Checker
    at ../go-cloud/samples/guestbook/main.go:313:6
//}


複数のProviderがあった場合は全てのProbiderSetの情報が表示されます。最後に、samples/guestbook内に定義されているInjector関数の一覧が表示されます。
これらの情報を確認すればあるProviderを使ってどんなオブジェクトが取得できるのか、またオブジェクトを取得したいときはBuilderでどんなオブジェクトを生成すればよいのかわかります。

=== wire check
wire checkコマンドは与えられたパッケージ内のProvider, Injectorの定義を検査します。
エラーがあった場合はそのエラー情報を出力しますし、エラーがなかった場合は何も表示せずに終了します。
正常実行時の出力がでないだけで、内部処理はwire showコマンドと同じです。


== Providerの実装を読み解く
Providerの概要は次の箇条書きのとおりです。

 * @<code>{wire.NewSet}でコンポーネントをまとめたProviderを定義できる。
 * @<code>{wire.NewSet}には他のProviderを含めることもできる
 * 関数の戻り値をインターフェースとして扱いたい場合は@<code>{wire.Bind}関数を使う
 * 後処理が必要なオブジェクトを提供する場合は、戻り値に@<code>{func}を含める

まずProviderの定義済みのコードを確認してみましょう。
@<list>{gcpcloud_services}は@<hd>{pro_and_inj}の冒頭でも引用したGCPの一連のサービスのコンポーネント群を生成できるProviderです。

//list[gcpcloud_services][go-cloud/gcp/gcpcloud/gcpcloud.goに定義されたProvider]{
var Services = wire.NewSet(
  cloudmysql.CertSourceSet,
  cloudmysql.Open,
  gcp.DefaultTransport,
  gcp.NewHTTPClient,
  runtimeconfigurator.Set,
  sdserver.Set)
//}


Providerを定義するときはグローバル変数で@<code>{wire.Set}関数を使って@<code>{wire.ProviderSet}オブジェクトの宣言をします。
@<code>{wire.Set}にはそのProviderで提供したいコンポーネントを返すビルド関数あるいは別の@<code>{wire.ProviderSet}を引数に与えます。
@<list>{gcpcloud_services}のProviderSetの場合、Cloud SQLへ接続する@<code>{*sql.DB}オブジェクトや@<code>{*requestlog.StackdriverLogger}オブジェクトを取得できます。
@<code>{wire.Set}の引数にしたビルド関数や別の@<code>{wire.ProviderSet}が必要とする入力を全てここで列挙する必要はありません。
例えば、@<list>{gcpcloud_services}で@<code>{wire.Set}の第2引数となっている@<code>{cloudmysql.Open}関数は@<list>{cloudmysql_open}のような宣言で@<code>{*sql.DB}オブジェクトを提供するビルド関数です。

//list[cloudmysql_open][go-cloud/mysql/cloudmysql/cloudmysql.goに定義されたcloudmysql.Open関数]{
// Open opens a Cloud SQL database.
func Open(ctx context.Context,
          certSource proxy.CertSource,
          params *Params) (*sql.DB, error) {
  // 省略
}
//}

第1引数の@<code>{context.Context}は@<code>{Services}内では生成されないオブジェクトなので、Injector作成時か@<code>{Services}を含んだ別の新しい@<code>{Provider}を定義するときに含めます@<fn>{context}。
第2引数の@<code>{proxy.CertSource}インターフェースは@<list>{gcpcloud_services}の@<code>{wire.Set}の第1引数となっている@<code>{cloudmysql.CertSourceSet}が提供しているので、
@<list>{gcpcloud_services}のProvider内で完結しています。

//footnote[context][実際に@<code>{context.Context}をProviderで用意することはないでしょう]

@<code>{cloudmysql.CertSourceSet}の宣言を見ると@<code>{cloudmysql_certsource}のようになっています。

//list[cloudmysql_certsource][go-cloud/mysql/cloudmysql/cloudmysql.goに定義されたCertSourceSet]{
// CertSourceSet is a Wire provider set that binds a Cloud SQL proxy
// certificate source from an GCP-authenticated HTTP client.
var CertSourceSet = wire.NewSet(
    NewCertSource,
    wire.Bind((*proxy.CertSource)(nil), (*certs.RemoteCertSource)(nil)))
//}

@<code>{NewCertSource}は@<code>{*certs.RemoteCertSource}オブジェクトを返すビルド関数ですが、@<code>{cloudmysql.CertSourceSet}では@<code>{proxy.CertSource}インターフェースを提供します。
そのため、 @<code>{wire.Bind}関数を用いて@<code>{*certs.RemoteCertSource}を@<code>{proxy.CertSource}インターフェースとして返す定義をしています。


== Injectorを自作する
@<hd>{wire_show}でwire showコマンドを使えば既成のProviderの提供情報を一覧できることは確認しました。。wire showコマンドの情報をを参考にInjectorを簡単に自作してみます。
Injectorの概要は次の箇条書きのとおりです。

 * wire.Buildを内部でよぶ。
 * Injectorメソッドのメソッド名に特別な制限はない
 * Injectorメソッドの引数はwire.Buildで呼ぶProviderで用意できない情報
 * Injectorメソッドの戻り値は以下の4種類しか定義できない
 ** Injectorで生成したいOutput
 ** Injectorで生成したいOutput、func()
 ** Injectorで生成したいOutput、error
 ** Injectorで生成したいOutput、func(), error
 * あるオブジェクト変数をインターフェースとして扱いたい場合は@<code>{wire.InterfaceValue}関数を使う

@<code>{func()}はProviderが@<code>{Close()}などの関数を一緒に返す時に強制されます。
たとえば、RDS MySQLのProviderは@<code>{db.Close}関数を呼んでcleanupするための関数を返します。
//list[open_return_func][mysql/rdsmysql/rdsmysql.Open関数]{
func Open(ctx context.Context, provider CertPoolProvider, params *Params) (*sql.DB, func(), error)
  // ...
  return db, func() { db.Close() }, nil
}
//}

ある@<code>{struct}オブジェクトを特定のインターフェース値としてProviderに注入する場合は@<code>{wire.InterfaceValue}関数を使います。
@<code>{os.Stdin}を@<code>{io.Reader}として注入する場合は@<code>{wire.Build}関数内で以下のように宣言します。

//list[interface_value][wire.InterfaceValueの例]{
wire.Build(wire.InterfaceValue(new(io.Reader), os.Stdin))
//}

では実際にInjectorを一つ実装してみます。
@<hd>{how_to_wire}で確認した@<code>{samples/guestbook}パッケージのapplicationSetの情報は@<list>{cmd_wire_show}です。

//list[cmd_wire_show][s]{
$ wire show
"github.com/google/go-cloud/samples/guestbook".applicationSet
...
Outputs given *database/sql.DB:
  []github.com/google/go-cloud/health.Checker
    at .../go-cloud/samples/guestbook/main.go:313:6
...
//}

@<code>{applicationSet}から@<code>{[]health.Checker}を生成したい場合は@<code>{*sql.DB}が必要でした。Injectorは@<list>{setup_checker}のような宣言になります。

//list[setup_checker][[\]health.Checkerを取得するためのInjector]{
func setupChecker(db *sql.DB) ([]health.Checker, func()) {
    // This will be filled in by Wire with providers from the provider sets in
    // wire.Build.
    wire.Build(
        applicationSet,
        // Injector引数で*sql.DBを用意しない場合は*sql.DBを生成する関数をここにセットする
    )
    return nil, nil
}
//}

Providerが要求する@<code>{*sql.DB}はInjectorの引数として渡すか、wire.Buildの引数で別の関数で生成できるようにします。
戻り値は自動生成されるので特に指定する必要はありません。

@<code>{func()}を第二戻り値にしているのは@<code>{[]health.Checker}のProviderが@<code>{cleanup()}関数を一緒に返すためです。
が、これを知らなくても@<code>{wire gen}コマンド実行時にエラーメッセージで指摘してくれます。

//cmd{
$ wire
/Users/budougumi0617/go/src/github.com/google/go-cloud/samples/guestbook/inject_gcp.go:53:1: inject setupChecker: provider for []github.com/google/go-cloud/health.Checker returns cleanup but injection does not return cleanup function
wire: generate failed
//}

このInjector定義からwire_gen.goには@<list>{build_gen_setup_checker}のようなコードが生成されます。

//list[build_gen_setup_checker][wire genコマンドで生成されたInjector]{
func setupChecker(db *sql.DB) ([]health.Checker, func()) {
    v, cleanup := appHealthChecks(db)
    return v, func() {
        cleanup()
    }
}
//}

このような小規模なDIでは何もメリットを感じられません。
が、@<hd>{pro_and_inj}でも引用した@<code>{*application}オブジェクトを取得するための@<code>{setupGCP}関数の定義から
自動生成される@<code>{*application}の初期化コードが@<list>{setup_gcp_gen}です。省略していますが、50行強のコードになります。
注入するオブジェクトがBLOBクライアント、 SQLクライアント(と、ロガーなどの設定)のみでもかなりのコード量が自動生成されることを考えると
必要なコンポーネントさえInjectorに並べる@<fn>{fn_last}だけでコーディングが済むのはかなり魅力的です。

//list[setup_gcp_gen][GCPに依存した*applicationオブジェクトを取得するInjector]{
func setupGCP(ctx context.Context,
              flags *cliFlags) (*application, func(), error) {
  stackdriverLogger := sdserver.NewRequestLogger()
  roundTripper := gcp.DefaultTransport()
  credentials, err := gcp.DefaultCredentials(ctx)
  if err != nil {
    return nil, nil, err
  }
  tokenSource := gcp.CredentialsTokenSource(credentials)
  httpClient, err := gcp.NewHTTPClient(roundTripper, tokenSource)
  if err != nil {
    return nil, nil, err
  }
  remoteCertSource := cloudmysql.NewCertSource(httpClient)
  projectID, err := gcp.DefaultProjectID(credentials)
  ...
  // このあとblob.Bucketなどの初期化が始まる
  ....
//}


//footnote[fn_last][@<code>{wire.Build}関数にProviderを並べるときもとくに依存関係順にしなくてはいけないなどの条件はありません。]

== おわりに
今回はgo-cloudリポジトリと付属するWireコマンドの概要を確認しました。
また、サンプルコードから@<code>{go-cloud/wire}パッケージを使ってDIを行う流れを解説しました。
Wireコマンドはgo-cloudのパッケージに依存しないDIツールでそれ単独でも利用することができます。
go-cloudを使わなずともGoでDIを行う時にWireを使うのはいかがでしょうか。

