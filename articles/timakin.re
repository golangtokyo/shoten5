= Go Modules導入以前

== はじめに

株式会社Gunosyのバックエンド兼フロントエンドエンジニアの@timakin@<fn>{fn1}です。
読者の皆様でも、もしかしたらご存知かもしれませんが、Goのバージョン1.11から、Modulesという依存パッケージ管理機構が導入されました。
@<code>{mod}というコマンドを通じてパッケージの管理ができます。もちろん、あくまで試験導入であり、正式に仕様が固まったという段階ではありません。

しかし、それを差し引いても、@<code>{glide}や@<code>{dep}などの既存パッケージ管理ツールを主に利用してきた人なら、
コマンドそのもののインストールや、@<code>{go get}だけでは解決しない依存関係という、
多少気になっても明確な解決策がなかった問題を、Modulesを通じて解決することにメリットを感じることでしょう。
本章では、そんなModulesについて実戦投入を前提としつつ、いくつかの角度から掘り下げていきます。

//footnote[fn1][@<href>{https://twitter.com/__timakin__}]

== Modules以前

=== 従来の依存パッケージ管理

Modulesについて書く前に、Modulesの意義を知るためにも、従来の依存パッケージはどのように行われていたのかを振り返る必要があります。
これまで、Goのプロジェクトで外部のパッケージへの依存関係を固定するためのツールはいくつも登場していました。

具体名を挙げるなら、非公式ではあるものの、有力な候補として活躍していたのは、@<code>{Godep}、@<code>{gb}、@<code>{glide}などでしょう。
また、2016年以降はほぼ公式のパッケージ管理ツールとして、@<code>{dep}が使われていました。
@<code>{dep}が登場して以降は、@<code>{Godep}や@<code>{glide}は開発こそ継続していたものの、@<code>{dep}リリース直後からREADMEで@<code>{dep}への移行を促す表明をしていました。
@<code>{dep}も、リリース当初は実験的な準公式ツールとしての色が強かったのですが、このように@<code>{glide}などの過去の有力ツールたちが一斉に移行を促したことで、一気に@<code>{dep}の導入が各所で行われました。

では、直近で準公式として活用されていた@<code>{dep}について見ていきましょう。
@<code>{dep}は直接の依存パッケージについてのマニフェストファイルである@<code>{Gopkg.toml}と、直接の依存パッケージがさらに依存するものを深ぼって記録し、プロジェクトを完全に再現できるようにした@<code>{Gopkg.lock}というファイルの2つを生成します。設定ファイルに公式がtoml形式を使うあたりに好感が持てますね。
それぞれ@<list>{list1}と@<list>{list2}のような中身で、これを元にプロジェクトルートにvendorディレクトリを生成します。

//list[list1][Gopkg.tomlの例]{
[[constraint]]
  name = "github.com/fukata/golang-stats-api-handler"
  version = "1.0.0"

[[constraint]]
  name = "github.com/go-chi/chi"
  version = "3.3.2"

[[constraint]]
  name = "github.com/go-sql-driver/mysql"
  version = "1.3.0"
//}

//list[list2][Gopkg.lockの例]{
[[projects]]
  digest = "1:289dd4d7abfb3ad2b5f728fbe9b1d5c1bf7d265a3eb9ef92869af1f7baba4c7a"
  name = "github.com/BurntSushi/toml"
  packages = ["."]
  pruneopts = ""
  revision = "b26d9c308763d68093482582cea63d69be07a0f0"
  version = "v0.3.0"

[[projects]]
  branch = "master"
  digest = "1:354e62d5acb9af138e13ec842f78a846d214a8d4a9f80e578698f1f1565e2ef8"
  name = "github.com/armon/go-metrics"
  packages = ["."]
  pruneopts = ""
  revision = "3c58d8115a78a6879e5df75ae900846768d36895"
//}

この@<code>{dep}は未だ現役で、今回紹介する@<code>{mod}コマンドを使うのがためらわれる、という方はこちらを使えば現状ベストな形で対応できると思います。

=== 従来のパッケージ管理で生じる課題

さて、従来のツールの名前をいくつか出してきましたが、これらのツールだといくつかの課題にぶつかる場面がありました。
ツールが外出しされている点です。標準ツールではないため、Docker利用時やCI時にコマンドをインストールするための処理を記述しなければなりませんでした。
@<code>{go get}コマンドをベースとして依存パッケージが管理されさえすればこの課題は乗り越えることができますが、
現状では何らかの形で外部のツールとしてインストールし、CIのビルドを進める必要がありました。

さらに、@<code>{dep}以外のコマンドについても、特定パッケージのみのアップデートができなかったり、
パッケージによってはそもそもダウンロードすることができないなどの不具合もありました。
ダウンロード時にローカルのキャッシュが効きすぎてしまい、手元で工夫しないと意図したバージョンのパッケージがダウンロードできない、などのバグも発生しました。

== Modulesの登場

=== Go & Versioning

従来の各ツールがこのような問題を抱える中、2018年初頭、Go & Versioning@<fn>{fn2}という一連の記事が公開されました。
これはRuss Cox氏によるGoの標準的なバージョン管理方法に関する提案記事で、先日リリースされたGo1.11の@<code>{mod}の原型となるアイデアが記載されています。
この記事群が公開された時点では、このProposalの実装は@<code>{vgo}コマンドとして実装されており、goコマンドの配下ではなく別コマンドとして位置付けられていました。

//footnote[fn2][@<href>{https://research.swtch.com/vgo}]

主に@<code>{mod}と共通している点としては、セマンティックバージョニング、gitのtagによる変更の適用や、GOPATHを意識しなくてもビルドが可能な点でしょうか。

特にセマンティックバージョニングについては、importを行うパッケージ利用者側だけではなく、パッケージ開発者も意識する必要があります。
仮にとある開発者がv1.0.1のパッケージを公開していたとして、後方互換性を伴うバグ修正をした場合はv1.0.2にし、同じく後方互換性がありつつ、機能の修正を行う場合はv1.1.0にします。
さらに、後方互換性を伴わない、そのまま置き換えてしまうとバグを生じうる修正の場合はv2.0.0にします。
ここで、仮にパッケージがメジャーアップデートしていた場合、パッケージ開発者はgitのtagをベースにバージョンを指定するとともに、
後方互換性を伴わない機能の変更が入ったパッケージに関しては、メジャーバージョンと同じ番号を持ったパス配下に置く必要があります。

例えば、v1であれば@<code>{/foo/bar}はそのままでいいですが、仮にメジャーアップデートが生じた場合、@<code>{/v1/foo/bar}と@<code>{/v2/foo/bar}を分けて開発する必要があります。
Goのパッケージではなかなか大幅なメジャーアップデートを行うものは見かけませんが、例えばJavaScriptのライブラリのように、
メジャーバージョンをいきなり大幅に上げるような振る舞いは、あまりよろしくないものとなります。

特にメジャーバージョンを細かく刻んでアップデートするとさらに大変で、そのバージョンごとにパッケージを切って開発する必要が出てくるでしょう。
このように、開発者は今までよりも明確にバージョンを意識しながら変更を加えていかなければなりません。

=== modの誕生

@<code>{vgo}がそのままGo1.11に正式導入されると当初は考えられてましたが、正式リリースまで若干の変更が加えられ続け、
@<code>{mod}というコマンドで標準パッケージとして試験導入されました。
@<code>{vgo}の最初のProposal時点からの差分としては、vendorディレクトリを完全にスルーする機構ではなく、GoのAPIがトップレベルでvendorディレクトリのサポートを行い、
プロジェクトのローカルへのダウンロードコマンドを設けた点、@<code>{go get -u=patch}コマンドを通じて依存パッケージを最新に更新できるようになった点など、より自然な形でGoの標準パッケージとして組み込まれる流れになりました。

== Modules導入

=== Modulesの利用方法

これまでModulesに至るまでの経緯や、Modulesの概要を見てきましたが、本節ではModulesを実際に利用してみた例を元に解説していこうと思います。
まず、Modulesの利用方法ですが、@<code>{mod}コマンドとして実装されているのもあり、いくつかオプションがあります。
Go1.11をインストールし、ターミナル上で@<code>{go mod help}と打つと、@<list>{list3}のような表示がされるかと思います。

//list[list3][go modのオプション一覧]{
download    download modules to local cache
edit        edit go.mod from tools or scripts
graph       print module requirement graph
init        initialize new module in current directory
tidy        add missing and remove unused modules
vendor      make vendored copy of dependencies
verify      verify dependencies have expected content
why         explain why packages or modules are needed
//}

中でも、@<code>{go mod init}や@<code>{go mod tidy}あたりは必ず使うでしょう。
どうしてもCIビルドでキャッシュしたい場合などでは、@<code>{go mod download} を使ってローカルにvendorディレクトリを作るというオプションの利用方法があります。

この@<code>{mod}コマンド、試験導入なこともありまだ浸透していないかと思いきや、大きめのOSSプロジェクトでも利用していたりします。
具体的にはOSSになったソースコード探索サービス、Sourcegraphの中で使われていたりします。@<fn>{fn3}

こちらの事例は非常に参考になるとは思いますが、１から進めていったときにどのような作業が必要なのか知るためにも、
自分自身でModulesの機構に則りながら、APIを書いてみました。当事例のパッケージ名をdratiniと呼称します。
こちらは実際に私のGithubのプロジェクトとして開発しましたが、内容について詳しい言及はいたしません。

//footnote[fn3][@<href>{https://github.com/sourcegraph/sourcegraph}]

まず前提として、Modulesは試験導入と再三書いた通り、GOPATH配下だとデフォルトで有効ではありません。
@<code>{GO111MODULE=on}という環境変数を設定する必要があります。GOPATH配下ではない、全く関係のないディレクトリ配下だった場合は、このオプションはデフォルトでONになり、
常に@<code>{build}や@<code>{test}の際に依存パッケージの存在確認を行って、なければ@<code>{GO111MODULE}オプションが有効になり、依存パッケージのインストールが行われます。

このオプションが有効になった状態で、まずは依存パッケージのマニフェストファイルを初期化するために、@<code>{go mod init}を実行します。
すると、@<code>{go.mod}ファイルが生成されると思います。中身はシンプルに、@<list>{list4}のような状態になっているかと思います。

//list[list4][初期化時のgo.mod]{
module github.com/timakin/dratini
//}

開発を進めていき、このマニフェストの内容以外にも依存パッケージの設定を追加しなければならない場合は、@<code>{go mod tidy}を実行します。
@<code>{dep}に慣れしたんでいた方は@<code>{dep ensure}を思い浮かべるとわかりやすいでしょう。
@<code>{tidy}は依存パッケージの追記や、使っていない依存関係を削除する役割を担います。

実行すると、@<code>{go.mod}ファイルへの依存パッケージ情報の追記と同時に、@<code>{go.sum}というファイルが新しく作成され、
依存パッケージの細かいchecksum情報が記載され、再現性が取れる形でどの時点の該当パッケージをインストールすればいいか出力します。
具体的には@<list>{list5}と@<list>{list6}のようなファイルになります。
このファイルをコミットして、別の人がそのプロジェクトで依存パッケージをインストールしたい場合は、@<code>{go mod download}などをしてローカルにvendorディレクトリを作りつつパッケージを持ってくるか、
@<code>{go build}や@<code>{go test}を実行する際に@<code>{GO111MODULE=on}を指定して

//list[list5][tidy実行後のgo.mod]{
module github.com/timakin/dratini

require (
	github.com/BurntSushi/toml v0.3.0
	github.com/RobotsAndPencils/buford v0.12.0
	github.com/client9/reopen v1.0.0
	github.com/davecgh/go-spew v1.1.1 // indirect
	github.com/pkg/errors v0.8.0
	github.com/pmezard/go-difflib v1.0.0 // indirect
	github.com/stretchr/testify v1.2.2
	go.uber.org/atomic v1.3.2 // indirect
	go.uber.org/multierr v1.1.0 // indirect
	go.uber.org/zap v1.9.1
	golang.org/x/crypto v0.0.0-20180910181607-0e37d006457b // indirect
	golang.org/x/net v0.0.0-20180906233101-161cd47e91fd
	golang.org/x/text v0.3.0 // indirect
)
//}

//list[list6][go.sumの出力結果]{
github.com/BurntSushi/toml v0.3.0 h1:e1/Ivsx3Z0FVTV0NSOv/aVgbUWyQuzj7=
github.com/BurntSushi/toml v0.3.0/go.mod h1:xHWCNGjB5oqiDr8zfno3MHue2Ht5sIBk=
github.com/RobotsAndPencils/buford v0.12.0 h1:2nfOk+N/QVoQHwXIS0m5TFdvlUjEnqAj=
github.com/RobotsAndPencils/buford v0.12.0/go.mod h1:27KhJZ/wLQHRnsZF+mTWKvF5w8U=
github.com/client9/reopen v1.0.0 h1:8tpLVR74DLpLObrn2KvsyxJY++2iORGR=
github.com/client9/reopen v1.0.0/go.mod h1:caXVCEr+lUtoN1FlsRiOWdfQtdRHIYfc=
github.com/davecgh/go-spew v1.1.1 h1:vj9j/u1bqnvCEfJOwUhtlOARqs3+rkHY=
github.com/davecgh/go-spew v1.1.1/go.mod h1:J7Y8YcW2NihsgmVo/mv3lAwl/skON4iL=
github.com/pkg/errors v0.8.0 h1:WdK/asTD0HN+q6hsWO3/vpuAkAr+tw6a=
github.com/pkg/errors v0.8.0/go.mod h1:bwawxfHBFNV+L2hUp1rHADufV3IMtnDR=
github.com/pmezard/go-difflib v1.0.0 h1:4DBwDE0NGyQoBHbLQYPwSUPoCMWR5BEz=
github.com/pmezard/go-difflib v1.0.0/go.mod h1:iKH77koFhYxTK1pcRnkKkqfTogsbg7gZ=
github.com/stretchr/testify v1.2.2 h1:bSDNvY7ZPG5RlJ8otE/7V6gMiyenm9Rt=
github.com/stretchr/testify v1.2.2/go.mod h1:a8OnRcib4nhh0OaRAV+Yts87kKdq0PP7=
go.uber.org/atomic v1.3.2 h1:2Oa65PReHzfn29GpvgsYwloV9AVFHPDk=
//}

=== Dockerイメージの作成やCI環境について

ここまでの流れを見ただけでは、Modulesの機構のメリットはただコマンドが標準パッケージに統合されただけに感じるかもしれません。
しかし、Dockerコンテナ上でのバイナリビルドなど、依存パッケージを基にした@<code>{build}コマンドの実行時には、Modulesのメリットを強く実感できます。
従来のバージョンでは、Goがデフォルトで入ったDockerイメージであれば@<code>{go get}で、そうでなければ@<code>{curl}や@<code>{apt-get}でコマンドを入れる必要があり、ビルドステップとしては本来不要なものが混ざってきます。

Modulesを使ったイメージ作成を実現する場合、@<list>{list7}のように、特に明示的なツールのインストールをすることなく、ビルドが可能になります。
ここで特徴的なのが、@<code>{WORKDIR}を@<code>{~/github.com/timakin/dratini}と、@<code>{GOPATH}を避けた形で設定している点です。
こうすることで、デフォルトで@<code>{GO111MODULE}フラグが有効になるので、ビルドステップの中で自動で依存パッケージがインストールされます。

//list[list7][Modules依存のDockerfileの例]{
ROM golang:1.11.0 AS builder
ADD . ~/github.com/timakin/dratini
WORKDIR ~/github.com/timakin/dratini
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -o app

FROM alpine:latest as runner
COPY --from=builder ~/github.com/timakin/dratini/app /usr/local/bin/app
//}

また、Dockerビルドを活用する場面で有力な候補としては、CircleCI 2.0でのビルドでしょうか。
その場合、ソースをGithubからチェックアウトしてきてテストを実行するまでに必要な設定は、@<list>{list8}のようなものだけです。

//list[list8][Modules依存のCircleCIのジョブ実行用configファイル]{
version: 2
jobs:
  build_and_test:
    working_directory: ~/github.com/timakin/dratini
    docker:
      - image: circleci/golang:1.11
    steps:
      - checkout
      - run: go test -race
//}

非常に短くシンプルに書けていると思います。もちろんこれは@<code>{GOPATH}配下を避けてチェックアウトしてるから実現するステップですが、
フラグを有効にしさえすればここまでシンプルに、Go自体が勝手に依存パッケージをインストールしてくれるというのは、見てて気持ちの良いものです。
さらに、@<code>{go get}でインストールできるソースであれば問題なく入るので、従来の他のツールで@<code>{git tag}やプライベートリポジトリ関係のバグが発生し、
パッケージがインストールできないなどの不具合もありません。

== おわりに

本章では、Go1.11で新たに導入されたModulesを取り巻く背景や、その利点、実際の使用例を見てきました。
外部ツールではなく標準搭載のツールとして@<code>{mod}コマンドが誕生したことで、ビルドステップが非常に簡潔に記述できることがおわかりいただけたと思います。
実際のOSSプロジェクトでも利用され始めているので、是非この機会にModulesにキャッチアップし、ご自身のプロジェクトに導入してみて欲しいと思います。
