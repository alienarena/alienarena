#!/usr/bin/env bash

# simple markdown-to-html converter
# usage: ./convert_md_to_html.sh input.md [output.html]
# if output is not given, uses input.md -> input.html

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "usage: $0 file.md [file.html]" >&2
    exit 1
fi

infile="$1"
outfile="${2:-${infile%.md}.html}"

# Create HTML wrapper with UTF-8 declaration, then process markdown
{
    echo '<!DOCTYPE html>'
    echo '<html lang="en">'
    echo '<head>'
    echo '<meta charset="UTF-8">'
    echo '<meta name="viewport" content="width=device-width, initial-scale=1.0">'
    echo '<title>Document</title>'
    echo '</head>'
    echo '<body>'
    
    # process the markdown
    awk '
    BEGIN { ul=0; ol=0; in_code=0 }

    function close_lists() {
        if (ul) { print "</ul>"; ul=0 }
        if (ol) { print "</ol>"; ol=0 }
    }

    function esc(str) {
        gsub(/&/, "MARKER_AMP", str)
        gsub(/</, "MARKER_LT", str)
        gsub(/>/, "MARKER_GT", str)
        return str
    }

    # code block delimiters (```)
    /^```/ {
        if (in_code) {
            print "</code></pre>"
            in_code=0
        } else {
            close_lists();
            print "<pre><code>"
            in_code=1
        }
        next
    }

    # when inside a code block, output literally (escape HTML)
    { if (in_code) { print esc($0); next } }

    # handle header levels 1-6
    /^(#{1,6})[ \t]+/ {
        close_lists();
        level = length($1)
        sub(/^#{1,6}[ \t]+/, "")
        printf("<h%d>%s</h%d>\n", level, esc($0), level)
        next
    }

    # unordered list
    /^[ \t]*[-*+][ \t]+/ {
        if (!ul) { close_lists(); print "<ul>"; ul=1 }
        sub(/^[ \t]*[-*+][ \t]+/, "")
        printf("  <li>%s</li>\n", esc($0))
        next
    }

    # ordered list
    /^[ \t]*[0-9]+\.[ \t]+/ {
        if (!ol) { close_lists(); print "<ol>"; ol=1 }
        sub(/^[ \t]*[0-9]+\.[ \t]+/, "")
        printf("  <li>%s</li>\n", esc($0))
        next
    }

    # blank line closes lists
    /^[ \t]*$/ {
        close_lists()
        print ""
        next
    }

    # horizontal rule (three or more dashes) -> <hr>
    /^[ \t]*-{3,}[ \t]*$/ {
        close_lists()
        print "<hr>"
        next
    }

    # paragraphs
    {
        close_lists();
        print "<p>" esc($0) "</p>"
    }

    END { close_lists() }
    ' "$infile" | \
        sed -E \
            -e 's/\*\*([^\*]+)\*\*/<strong>\1<\/strong>/g' \
            -e 's/`([^`]+)`/<code>\1<\/code>/g' \
            -e 's/MARKER_AMP/\&amp;/g' \
            -e 's/MARKER_LT/\&lt;/g' \
            -e 's/MARKER_GT/\&gt;/g'
    
    echo '</body>'
    echo '</html>'
} > "$outfile"

echo "converted $infile -> $outfile"
