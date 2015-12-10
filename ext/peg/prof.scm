; 以下で実行
; gosh prof.scm
(use gauche.vm.profiler)
(use parser.peg)
(use text.csv)

(define data
  (let1 d (call-with-input-file "data/13tokyo.csv"
            port->string
            :encoding 'utf8)
    ; test dataっぽい 5setってこと?
    ; (string-append d d d d d)))
    (string-append d)))

; csvの場合、"文字列", の扱いが実装によって異なるので注意すること
; , "string" HOGE HOGE ,
; , HOGE "string" ,
; みたいな場合(unquoted, quoted が混在してるけどどうなんの?みたいな)

(define csv-parser
  (let* ((spaces  ($many ($one-of #[ \t])))
         (skip-spaces ($skip-many ($one-of #[ \t])))
         (comma ($seq skip-spaces ($char #\,) skip-spaces))
         (dquote ($char #\"))
         (double-dquote ($do (($string "\"\"")) ($return #\")))
         ; "文字列"  "が連続した場合、は"にして返す
         ; "以外の文字列を文字とみなしてる
         (quoted-body ($many ($or ($one-of #[^\"]) double-dquote)))
         ; "文字列"のパース部分ね
         (quoted ($between dquote quoted-body dquote))
         ; 空白文字とカンマ以外 :ERROR: \rが抜けてる気がする
         (unquoted ($alternate ($one-of #[^ \t\n,]) spaces))
         (field ($or quoted unquoted))  ; "で囲まれてるかいなか
         ; 少なくとも1つ$->ropeで文字列の遅延だっけか
         (record ($sep-by ($->rope field) comma 1)))
    ($sep-by record newline)))

; profileの記述の仕方
(profiler-start)
(peg-parse-string csv-parser data)
(profiler-stop)
(profiler-show)
(profiler-reset)

; text.csvを使ったparserみたい(10倍くらい速いみたい)
(define reader (make-csv-reader #\,))

(profiler-start)
(call-with-input-string data (^p (generator-for-each identity (cut reader p))))
(profiler-stop)
(profiler-show)


