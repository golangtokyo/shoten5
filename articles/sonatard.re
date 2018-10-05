
= HTTPパケットが飛んでいくまで

== はじめに


ネクストカレンシー株式会社でバックエンドエンジニアをしているsonatard@<fn>{sonatard_fn1}です。



//footnote[sonatard_fn1][@<href>{https://twitter.com/sonatard}]



本章では、GoのHTTPリクエストの送信処理の概要を紹介します。



以下のような関数は、Goで開発している方なら多くの方がお世話になっている関数だと思います。
しかし最終的にどのようにパケットが送られるかご存知でしょうか？本章では、そのゴールを突き止めたいと思います。


//emlist{
resp, err := http.Get(url)
resp, err := http.PostForm(url, values)
//}

== HTTPリクエストの送信シーケンス

=== 送信処理の概要


以下がHTTPリクエストのシーケンス図になります。
以降の章は、このシーケンスと比較しながら読んでみてください。



//image[http_seq][HTTPリクエストシーケンス]{
//}



=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/client.go#L369-L371,func Get(url string) (resp *Response\, err error)}


ユーザが自身で @<tt>{http.Clinet{\}} を作成しなかった場合に利用する関数です。



http package内で初期化されている @<tt>{DefaultCliet} の@<tt>{Celint.Get()}を実行します。


//emlist{
// DefaultClient is the default Client and is used by Get, Head, and Post.
var DefaultClient = &Client{}

func Get(url string) (resp *Response, err error) {
    return DefaultClient.Get(url)
}
//}

=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/client.go#L393-L399,func (c *Client) Get(url string) (resp *Response\, err error)}


@<tt>{NewRequest} では、URLのパースやHostの設定などをして@<tt>{http.Request} を作成します。
作成した @<tt>{http.Request} を引数に渡して @<tt>{Client.Do()} を実行しています。


//emlist{
func (c *Client) Get(url string) (resp *Response, err error) {
    req, err := NewRequest("GET", url, nil)
    if err != nil {
        return nil, err
    }
    return c.Do(req)
}
//}

=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/client.go#L508-L510,func (c Client) Do(req *Request) (Response\, error)}

//emlist{
func (c *Client) Do(req *Request) (*Response, error) {
    return c.do(req)
}
//}

=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/client.go#L514-L662,func (c *Client) do(req *Request) (retres *Response\, reterr error)}


ここから具体的なHTTPリクエストの送信処理のための準備が始まります。
今まではHTTPのGETメソッドに依存した処理でしたが、ここからはPOSTなども共通に利用されます。



次に実行される @<tt>{Client.send} を実行しているだけですが、 ここではRedirectについて制御をしています。
レスポンスのステータスコードからRedirectが必要だと判断した場合には、@<href>{https://developer.mozilla.org/ja/docs/Web/HTTP/Headers/Location,Locationヘッダ}を読み、次のRequestのURLやHostとして設定されます。


//emlist{
func (c *Client) Do(req *Request) (*Response, error) {
    // リダイレクトされる限りループする
    for {
        // 1度でもRedirectされた場合
        if len(reqs) > 0 {
            // Locationヘッダの読み込みや次のRequestの作成
            // 省略
        }

        reqs = append(reqs, req)
        resp, didTimeout, err = c.send(req, deadline);
        // 省略
        redirectMethod, shouldRedirect, includeBody = redirectBehavior(req.Method, resp, reqs[0])
        if !shouldRedirect {
            return resp, nil
        }
    }
}
//}

=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/client.go#L168-L198,func (c *Client) send(req *Request\, deadline time.Time) (resp *Response\, didTimeout func() bool\, err error)}


Cookieの処理をして @<tt>{http.sned} を実行します。


//emlist{
// didTimeout is non-nil only if err != nil.
func (c *Client) send(req *Request, deadline time.Time) (resp *Response, didTimeout func() bool, err error) {
    if c.Jar != nil {
        for _, cookie := range c.Jar.Cookies(req.URL) {
            req.AddCookie(cookie)
        }
    }
    resp, didTimeout, err = send(req, c.transport(), deadline)
    if err != nil {
        return nil, didTimeout, err
    }
    if c.Jar != nil {
        if rc := resp.Cookies(); len(rc) > 0 {
            c.Jar.SetCookies(req.URL, rc)
        }
    }
    return resp, nil, nil
}
//}

=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/client.go#L202-L274,func send(ireq *Request\, rt RoundTripper\, deadline time.Time) (resp *Response\, didTimeout func() bool\, err error)}


ここでは、まず改めてhttp.Requestの値を設定します。http.Request構造体のnilの部分にメモリを確保とBasic認証のためにAuthorizationヘッダの設定を行なっています。
このとき元のrequestを更新してしまうと、Redirect時の次のループで影響するため更新する場合はコピーを行なってから更新しています。
http.Requestの準備ができたら送信処理の @<tt>{RoundTrip(req *Request) (*Response, error)} を実行します。
これは@<tt>{RoundTripper} Interfaceの @<tt>{RoundTripper} メソッドで、あり@<tt>{Client} の @<tt>{Transport} に設定することで異なる処理を実行することもできます。何も挟まなければ、@<tt>{net/http/transport.go} の @<tt>{func (t *Transport) RoundTrip(req *Request) (*Response, error)} が実行されます。


//emlist{
func send(ireq *Request, rt RoundTripper, deadline time.Time) (resp *Response, didTimeout func() bool, err error)
    req := ireq // req is either the original request, or a modified fork
    //省略

    // 1度だけコピーを行うための関数
    forkReq := func() {
        if ireq == req {
            req = new(Request)
            *req = *ireq // shallow clone
        }
    }

    if req.Header == nil {
        forkReq()
        req.Header = make(Header)
    }

    // Basic認証のヘッダを作成
    if u := req.URL.User; u != nil && req.Header.Get("Authorization") == "" {
        username := u.Username()
        password, _ := u.Password()
        forkReq()
        req.Header = cloneHeader(ireq.Header)
        req.Header.Set("Authorization", "Basic "+basicAuth(username, password))
    }
    // 省略

    resp, err = rt.RoundTrip(req)
    // 省略
}
//}

=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/roundtrip.go,func (t Transport) RoundTrip(req *Request) (Response\, error)}


Transport.RoundTripはRoundTrip Interfaceの実装です。
以下の通り @<tt>{Transport.roundTrip} を実行しています。


//emlist{
func (t *Transport) RoundTrip(req *Request) (*Response, error) {
    return t.roundTrip(req)
}
//}

=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/transport.go#L385-L492,func (t Transport) roundTrip(req *Request) (Response\, error)}


引数のhttp.Requestに基づいてヘッダのバリデーションやコネクションの管理をして、@<tt>{persistConn.roundTrip} を実行します。


//emlist{
func (t *Transport) RoundTrip(req *Request) (*Response, error) {
    // 省略
        // HTTPヘッダが正しいことを検証する
        for k, vv := range req.Header {
            if !httpguts.ValidHeaderFieldName(k) {
                return nil, fmt.Errorf("net/http: invalid header field name %q", k)
            }
            for _, v := range vv {
                if !httpguts.ValidHeaderFieldValue(v) {
                    return nil, fmt.Errorf("net/http: invalid header field value %q for key %v", v, k)
                }
            }
        }

    // 省略

    // errによってはリトライする
    for {
        // キャッシュされたコネクションの取得をするか、新たにコネクションを作成する。
        pconn, err := t.getConn(treq, cm)
        if err != nil {
            t.setReqCanceler(req, nil)
            req.closeBody()
            return nil, err
        }
        var resp *Response
        if pconn.alt != nil {
            // HTTP/2 path.
            t.setReqCanceler(req, nil) // not cancelable with CancelRequest
            resp, err = pconn.alt.RoundTrip(req)
        } else {
            resp, err = pconn.roundTrip(treq)
        }
        // 成功すればresponseを返すが、成功しなければforループにより再実行する
        if err == nil {
            return resp, nil
        }
        if !pconn.shouldRetryRequest(req, err) {
            // Issue 16465: return underlying net.Conn.Read error from peek,
            // as we've historically done.
            if e, ok := err.(transportReadFromServerError); ok {
                err = e.err
            }
            return nil, err
        }
       // 省略
    }
//}

=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/transport.go#L932-L1088,func (t Transport) getConn(treq *transportRequest\, cm connectMethod) (persistConn\, error)}


getConnではキャッシュされたTCPコネクションを取得するか、キャッシュがない場合には新たにTCPコネクションを確立します。
新規にコネクションを作成する際にはdialを実行しますが、既に確立されているコネクションの終了が先の場合にはそちらを使います。


//emlist{
func (t *Transport) getConn(treq *transportRequest, cm connectMethod) (*persistConn, error) {
    // 省略
    case pc := <-idleConnCh:
        // 省略
        return pc, nil

        // 省略
}
//}

=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/transport.go#L2020-L2149,func (pc *persistConn) roundTrip()}


persistConnでは、gzip圧縮の処理、送信、結果の待ち受けを行います。
この後の送信処理は非同期で行われるため別のchannelに渡すことになります。



こちらはユーザが実行したタスクで行われる送信処理の最深部になります。


//emlist{
func (pc *persistConn) roundTrip(req *transportRequest) (resp *Response, err error) {
    if !pc.t.replaceReqCanceler(req.Request, pc.cancelRequest) {
        pc.t.putOrCloseIdleConn(pc)
        return nil, errRequestCanceled
    }

    requestedGzip := false
    if !pc.t.DisableCompression &&
        req.Header.Get("Accept-Encoding") == "" &&
        req.Header.Get("Range") == "" &&
        req.Method != "HEAD" {
        requestedGzip = true
        req.extraHeaders().Set("Accept-Encoding", "gzip")
    }

    // 送信処理
    startBytesWritten := pc.nwrite
    writeErrCh := make(chan error, 1)
    pc.writech <- writeRequest{req, writeErrCh, continueCh}

    // 結果の待ち受け
    for {
        select {
        // 省略
        case <-respHeaderTimer:
            pc.close(errTimeout)
            return nil, errTimeout
        case re := <-resc:
            if re.err != nil {
                return nil, pc.mapRoundTripError(req, startBytesWritten, re.err)
            }
            return re.res, nil
        // 省略
        }
    }
}
//}


writeするchannleは以下の @<tt>{chan writeRequest} になります。


//emlist{
type persistConn struct {
    // 省略
    writech   chan writeRequest   // written by roundTrip; read by writeLoop
    // 省略
    mutateHeaderFunc func(Header)
}
//}


@<tt>{chan writech} を受けているのは、@<tt>{func (pc *persistConn) writeLoop()} になります。


=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/transport.go#L1882-L1919,func (pc *persistConn) writeLoop()}


@<tt>{writeLoop} は、ユーザ実行タスクから送信する @<tt>{http.Request} を Channel経由で受け取り送信処理を実行します。


//emlist{
func (pc *persistConn) writeLoop() {
    // 省略
    for {
        select {
        case wr := <-pc.writech:
            startBytesWritten := pc.nwrite
            err := wr.req.Request.write(pc.bw, pc.isProxy, wr.req.extra, pc.waitForContinue(wr.continueCh))
    // 省略
}
//}


このとき @<tt>{write} の 第1引数に渡している @<tt>{pc.pw} は以下になります。


//emlist{
type persistConn struct {
    // 省略
    bw        *bufio.Writer       // to conn
    // 省略
}
//}


@<tt>{writeLoop} の実行と @<tt>{bw *bufio.Writer} がいつ設定されるかというと @<tt>{getConn()} から呼ばれる @<tt>{dialConn()} の内部になります。


//emlist{
func (t *Transport) dialConn(ctx context.Context, cm connectMethod) (*persistConn, error) {
    // 省略
    pconn.bw = bufio.NewWriter(persistConnWriter{pconn})
    go pconn.writeLoop()
    return pconn, nil
}
//}


@<tt>{persistConnWriter} は、以下の通りです。
@<tt>{persistConn} の @<tt>{conn} に @<tt>{Write} していることがわかります。



@<tt>{conn} は、 @<tt>{net.Conn} になります。


//emlist{
type persistConnWriter struct {
    pc *persistConn
}

func (w persistConnWriter) Write(p []byte) (n int, err error) {
    n, err = w.pc.conn.Write(p)
    w.pc.nwrite += int64(n)
    return
}
//}

//emlist{
type persistConn struct {
    conn      net.Conn
}
//}


あとは @<tt>{func (req *Request) write(w io.Writer, usingProxy bool, extraHeaders Header, waitForContinue func() bool) (err error)} に渡している @<tt>{w} に対して実際にWriteしているところを見つければゴールです。


=== @<href>{https://github.com/golang/go/blob/go1.11.1/src/net/http/request.go#L514-L657,func (req *Request) write(w io.Writer\, usingProxy bool\, extraHeaders Header\, waitForContinue func() bool) (err error)}


ここでいよいよHTTPリクエストで実際に送信するテキストを組み立てていきます。



HTTP1はバイナリフォーマットではなくテキストフォーマットの珍しいプロトコルであるため、文字列にして @<tt>{w} に対して書き込んでいきます。
ここにたどり着くまでに送るべきデータの構造体は完成しているので、それをHTTPのHeaderやBodyデータに変換していきます。



net.ConnのWriteへの書き込みは @<tt>{*bufio.Writer} 型なので、バッファが行われるため、書き込んだタイミングで即座に送信されるというわけではありません。そのため必要なタイミングで @<tt>{flush} を実行しています。


//emlist{
func (req *Request) write(w io.Writer, usingProxy bool, extraHeaders Header, waitForContinue func() bool) (err error) {
    // 省略

    // メソッドを送信
    _, err = fmt.Fprintf(w, "%s %s HTTP/1.1\r\n", valueOrDefault(r.Method, "GET"), ruri)
    if err != nil {
        return err
    }

    // ヘッダを送信
    // Header lines
    _, err = fmt.Fprintf(w, "Host: %s\r\n", host)
    if err != nil {
        return err
    }

    // ユーザエージェントの送信
    userAgent := defaultUserAgent
    if _, ok := r.Header["User-Agent"]; ok {
        userAgent = r.Header.Get("User-Agent")
    }
    if userAgent != "" {
        _, err = fmt.Fprintf(w, "User-Agent: %s\r\n", userAgent)
        if err != nil {
            return err
        }
        if trace != nil && trace.WroteHeaderField != nil {
            trace.WroteHeaderField("User-Agent", []string{userAgent})
        }
    }

    _, err = io.WriteString(w, "\r\n")
    if err != nil {
        return err
    }
    // 省略

    // Bodyの書き込み
    // Write body and trailer
    err = tw.writeBody(w)
    if err != nil {
        if tw.bodyReadError == err {
            err = requestBodyReadError{err}
        }
        return err
    }

    if bw != nil {
        // バッファをフラッシュ、本当の送信
        return bw.Flush()
    }
}
//}

=== HTTPパケット送信シーケンスまとめ


Goではとても簡単にHTTPリクエストを送ることができますが、これはGoが標準ライブラリでこれだけの処理を実装してくれているためです。
net.ConnへのWriteからシステムコールのsocketのsendの実装は？
Responseはどのようにして受け取るのか?
など、まだ疑問は残りますが続きはみなさんで調べてみてください。


== 学び


次に送信の一連の処理を読んでいて気が付いた学びを紹介します。


=== Channelの実例


Channelのサンプルでよく登場するタイマーによるリクエストのキャンセル処理が書かれています。


//emlist{
func setRequestCancel(req *Request, rt RoundTripper, deadline time.Time) (stopTimer func(), didTimeout func() bool) {
    // 省略

    stopTimerCh := make(chan struct{})
    var once sync.Once
    stopTimer = func() { once.Do(func() { close(stopTimerCh) }) }

    timer := time.NewTimer(time.Until(deadline))
    var timedOut atomicBool

    go func() {
        select {
        case <-initialReqCancel:
            doCancel()
            timer.Stop()
        case <-timer.C:
            timedOut.setTrue()
            doCancel()
        case <-stopTimerCh:
            timer.Stop()
        }
    }()

    // 省略
}
//}

=== utility package


単純な文字列処理をするhttpgutsパッケージを利用しています。
httpgutsは、内部ではなく外部の @<tt>{golang_org/x/net/http/httpguts} ライブラリに分割しています。utilパッケージのようなものは否定されることもありますが、標準ライブラリでも行われていることがわかります。



このような汎用的な処理を、無理にドメインに配置するのではなく切り出すのは正しいということです。
ただしこのときに様々な機能を1つのutilily パッケージのようなものを作ることは避けるべきでしょう。


//emlist{
func (t *Transport) RoundTrip(req *Request) (*Response, error) {
    // 省略
        for k, vv := range req.Header {
            if !httpguts.ValidHeaderFieldName(k) {
                return nil, fmt.Errorf("net/http: invalid header field name %q", k)
            }
            for _, v := range vv {
                if !httpguts.ValidHeaderFieldValue(v) {
                    return nil, fmt.Errorf("net/http: invalid header field value %q for key %v", v, k)
                }
            }
        }
    // 省略
//}

=== ループのリトライ処理


いくつかでてきたがリトライ処理が必要なものは無限ループで、成功した場合にだけ抜けるという書き方をしています。


//emlist{
    for {
        // 省略
        resp, err = pconn.alt.RoundTrip(req)
        if err == nil {
        // 成功すればresponseを返すが、成功しなければforループにより再実行する
            return resp, nil
        }


//}

=== 1つの関数内でのみ実行される関数の定義場所


handlePendingDialなどその関数でしか使わない共通処理は、関数内部でクロージャとして定義されています。


//emlist{
func (t *Transport) getConn(treq *transportRequest, cm connectMethod) (*persistConn, error) {
        handlePendingDial := func() {
        testHookPrePendingDial()
        go func() {
            if v := <-dialc; v.err == nil {
                t.putOrCloseIdleConn(v.pc)
            } else {
                t.decHostConnCount(cmKey)
            }
            testHookPostPendingDial()
        }()
    }

    // 省略
        case pc := <-idleConnCh:
        handlePendingDial()
        if trace != nil && trace.GotConn != nil {
            trace.GotConn(httptrace.GotConnInfo{Conn: pc.conn, Reused: pc.isReused()})
        }
        return pc, nil
    case <-req.Cancel:
        handlePendingDial()
        return nil, errRequestCanceledConn
    case <-req.Context().Done():
        handlePendingDial()
        return nil, req.Context().Err()
    case err := <-cancelc:
        handlePendingDial()
        if err == errRequestCanceled {
            err = errRequestCanceledConn
        }
}
//}

=== 1度だけコピー


1度だけコピーしたいというテクニックの紹介です。


//emlist{
func send(ireq *Request, rt RoundTripper, deadline time.Time) (resp *Response, didTimeout func() bool, err error)
    // 初期状態のireqを保存
    req := ireq // req is either the original request, or a modified fork
    //省略

    // 1度だけコピーする処理
    forkReq := func() {
        // もし初期状態と変わっていなければ新しいRequestを作成してコピー
        if ireq == req {
            req = new(Request)
            *req = *ireq // shallow clone
        }
    }

    if req.Header == nil {
        // reqを書き換える前にコピー
        forkReq()
        req.Header = make(Header)
    }
    // 省略
}
//}

=== Hook関数を用意したテスト


テスト用にHook関数が実行挿入している。普段はnopなので、何もしませんがテストのときだけ挿入するようになっています。


//emlist{
func nop() {}

// testHooks. Always non-nil.
var (
    testHookEnterRoundTrip   = nop
)

func (pc *persistConn) roundTrip(req *transportRequest) (resp *Response, err error) {
    testHookEnterRoundTrip()
    if !pc.t.replaceReqCanceler(req.Request, pc.cancelRequest) {
        pc.t.putOrCloseIdleConn(pc)
        return nil, errRequestCanceled
    }
    pc.mu.Lock()
    pc.numExpectedResponses++
    headerFn := pc.mutateHeaderFunc
    pc.mu.Unlock()

    if headerFn != nil {
        headerFn(req.extraHeaders())
    }
//}


以下では、RoundTrip実行時にCnacelのRequestを送って正しくエラーが返ってくることを確認するテストしています。


//emlist{
    SetEnterRoundTripHook(func() {
        tr.CancelRequest(req)
    })
    defer SetEnterRoundTripHook(nil)
    res, err := tr.RoundTrip(req)
    if err != ExportErrRequestCanceled {
        t.Errorf("expected canceled request error; got %v", err)
        if err == nil {
            res.Body.Close()
        }
    }
//}

=== デバッグ用コードやテスト用コードはベタがき


素直にベタがきをしています。
さらに変数定義が関数内にあり、デバッグ時にtrueにして使うのだと思います。



正しさよりもシンプルな実装を心がけることについて、勇気を与えてくれるコードです。


//emlist{
func (pc *persistConn) roundTrip(req *transportRequest) (resp *Response, err error) {

    // 省略
    const debugRoundTrip = false

    // 省略
    if debugRoundTrip {
            req.logf("writeErrCh resv: %T/%#v", err, err)
    }
}
//}

== おわりに


本章では、普段からよく利用するHTTPリクエストの送信処理の紹介とそこからの学びを紹介しました。
中々この知識を業務で活かす機会は少ないかもしれませんが、何かのお役に立てば幸いです。



また学びについては、Goではシンプルに書くことが正しいとされていますが、
開発していると手の込んだ抽象化した設計を選定したくなりがちです。
しかし抽象化のコードは本質的なロジックの見通しが悪くなることがあります。
設計をする際には @<tt>{if debugRoundTrip {} の愚直さをを思い出して、バランスを取った実装をしていきたいものです。

