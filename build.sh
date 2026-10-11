#!/bin/sh
# Использование: build.sh <исходный_файл>
#
# Коды возврата:
#   0   - успех
#   1   - неверное использование
#   2   - исходный файл не найден или не читается
#   3   - нет комментария Output: или некорректное имя
#   4   - неподдерживаемый тип файла
#   5   - не удалось создать временный каталог
#   6   - не найден компилятор
#   7   - ошибка компиляции
#   8   - конечный файл не получен или не скопирован
#   129 - прерван сигналом HUP
#   130 - прерван сигналом INT
#   131 - прерван сигналом QUIT
#   143 - прерван сигналом TERM

TMP=""
CHILD=""

cleanup() {
    rc=$?
    trap - EXIT
    if [ -n "$CHILD" ]; then
        kill "$CHILD" 2>/dev/null
        wait "$CHILD" 2>/dev/null
    fi
    [ -n "$TMP" ] && rm -rf -- "$TMP"
    exit "$rc"
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 131' QUIT
trap 'exit 143' TERM

die() {
    code=$1
    echo "$2" >&2
    exit "$code"
}

run() {
    "$@" &
    CHILD=$!
    wait "$CHILD"
    st=$?
    CHILD=""
    return "$st"
}

tex_pass() {
    cd "$srcdir" && exec pdflatex -interaction=nonstopmode -halt-on-error \
        -output-directory="$TMP" -jobname=result "$srcfile" >"$TMP/tex.stdout" 2>&1
}

[ $# -eq 1 ] || die 1 "использование: $0 <исходный_файл>"

src=$1
[ -f "$src" ] && [ -r "$src" ] || die 2 "файл не найден или не читается"

srcdir=$(cd -- "$(dirname -- "$src")" && pwd) || die 2 "некорректный путь"
srcfile=$(basename -- "$src")
srcabs="$srcdir/$srcfile"

out=$(sed -n 's/.*Output:[[:space:]]*\([^[:space:]]*\).*/\1/p' "$srcabs" | head -n 1)
out=${out%\*/}

[ -n "$out" ] || die 3 "не найден комментарий Output:"
case $out in
    */*|.|..) die 3 "некорректное имя конечного файла" ;;
esac

TMP=$(mktemp -d "${TMPDIR:-/tmp}/build.XXXXXX") || die 5 "не удалось создать временный каталог"
TMPDIR=$TMP
export TMPDIR

case $srcfile in
    *.c)
        cc=""
        for c in cc gcc clang; do
            command -v "$c" >/dev/null 2>&1 && { cc=$c; break; }
        done
        [ -n "$cc" ] || die 6 "не найден компилятор"
        run "$cc" -I"$srcdir" -o "$TMP/$out" "$srcabs" || die 7 "ошибка компиляции"
        built="$TMP/$out"
        ;;
    *.cpp|*.cc|*.cxx)
        cxx=""
        for c in c++ g++ clang++; do
            command -v "$c" >/dev/null 2>&1 && { cxx=$c; break; }
        done
        [ -n "$cxx" ] || die 6 "не найден компилятор"
        run "$cxx" -I"$srcdir" -o "$TMP/$out" "$srcabs" || die 7 "ошибка компиляции"
        built="$TMP/$out"
        ;;
    *.tex)
        command -v pdflatex >/dev/null 2>&1 || die 6 "не найден компилятор"
        i=0
        while [ "$i" -lt 2 ]; do
            run tex_pass \
                || { cat "$TMP/tex.stdout" >&2; die 7 "ошибка компиляции"; }
            i=$((i + 1))
        done
        built="$TMP/result.pdf"
        ;;
    *)
        die 4 "неподдерживаемый тип файла"
        ;;
esac

[ -f "$built" ] || die 8 "конечный файл не получен"
cp -- "$built" "$srcdir/$out" || die 8 "не удалось скопировать результат"
