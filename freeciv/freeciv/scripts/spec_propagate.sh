#! /bin/sh -e
# Helper script for Freeciv development to propagate changes made to
# data files (usually comments describing the format) from one ruleset to all
# other rulesets (which generally have supposedly-identical copies of this
# material that should stay in sync).
# Must be run from the root of a git checkout.

usage() {
    echo "*** usage: $0 data/[changed-ruleset-dir]"
}

patch_rulesetdir() {
    P="`mktemp`"
    git diff "$1" >"$P"
    rej=""
    for r in data/*/game.ruleset; do
        d="`dirname $r`"
        # skip original dir
        case "$1" in
            "$d"|"$d/")
                echo "*** skipping original dir $d"
                ;;
            *)
                echo "*** patching ruleset $d"
                patch -d "$d" -p3 <"$P" || {
                    case "$?" in
                        1) rej="$rej $d" ;;
                        *) 
                            echo "*** patch was seriously unhappy with $d, giving up"
                            return
                            ;;
                    esac
                }
                ;;
        esac
    done
    rm "$P"
    if [ "x$rej" != x ]; then
        echo "*** fix up rejects manually: $rej"
    fi
    if [ -f "data/ruledit/comments.txt" ]; then
        echo "*** fix up ruledit manually: data/ruledit/comments.txt"
    fi
}

if [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

if [ ! -f "./fc_version" ]; then
    echo "*** Run me from Freeciv git root"
    usage
    exit 1
fi

case "$1" in
    data/*.tilespec)
        echo "*** FIXME: $1 looks like a tileset spec and I don't do those yet"
        usage
        exit 1
        ;;
    data/*)
        if [ ! -f "$1/game.ruleset" ]; then
            echo "*** $1 doesn't look like a ruleset dir (I don't do tileset dirs)"
            usage
            exit 1
        fi
        # else assume it's a ruleset directory
        patch_rulesetdir "$1"
        ;;
    *)
        echo "*** don't know how to propagate $1"
        usage
        exit 1
        ;;
esac
