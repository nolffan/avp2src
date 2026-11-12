#!/usr/bin/env bash
# The game directory as one file, to copy to the machine that plays it:
#
#   build/package.sh out/avp2-<legacy|modern>-<release|debug>
#
# writes two archives, each unpacking to <name>/ (AVP2/, PrimalHunt/ and the
# run scripts: build/all.sh):
#
#   out/<name>-private.tar.gz   the whole directory, the retail games' data
#                               included: for your own machines, and anyone
#                               developing on the build. Never to be
#                               published.
#   out/<name>-public.tar.gz    the same without anything that came from the
#                               retail games (each MANIFEST.txt's `data` and
#                               `THIRD-PARTY` files), which its
#                               linux-install.sh / windows-install.bat /
#                               mac-install.command copy out of the player's
#                               own disc images (build/mkinstall.py): the one
#                               to give to other people. It needs the images
#                               of the European and the USA releases' discs
#                               in rez/isos/ (build/retail.sh, retail_discs);
#                               without them it is skipped.
#
# AVP2_ARCHIVES (default "private public") says which to write; with
# "public" alone (the CI build, .github/workflows/public-archives.yml) the
# private one is not made, and no disc images is an error, not a skip.
#
# build/all.sh runs it for the release build (step 6) and build/variant.sh
# for each variant, once its symbols are in place.
#
# Links are stored as the files they point to (-h). Each archive is a fresh
# install, whatever has been played in the directory: no saves (empty
# Save/), no logs, no player profiles but the shipped default ones,
# autoexec.cfg as the shipped one (autoexec.cfg.retail), avp2cmds.txt as the
# build writes it (build/avp2cmds.sh), and none of the per-run files the run
# scripts write (run-cmds.txt); the public one has neither autoexec.cfg nor
# the default profiles, which its install scripts write. Each is written
# beside and renamed into place, so a failed run never leaves a truncated
# one.
set -e
OUT="${1:?usage: build/package.sh <game directory>}"
OUT="${OUT%/}"
NAME="$(basename "$OUT")"
PARENT="$(dirname "$OUT")"
PKG="$PARENT/$NAME-private.tar.gz"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCHIVES=" ${AVP2_ARCHIVES:-private public} "
GAMES=(AVP2 PrimalHunt)

# The fresh files, staged beside the directory and renamed into it.
STAGE=".pkg-$NAME"
rm -rf "$PARENT/$STAGE"
mkdir -p "$PARENT/$STAGE"
trap 'rm -rf "$PARENT/$STAGE"' EXIT
FRESH=()
for g in "${GAMES[@]}"; do
    mkdir -p "$PARENT/$STAGE/$g/Profiles"
    cp "$OUT/$g/autoexec.cfg.retail" "$PARENT/$STAGE/$g/autoexec.cfg"
    cp "$OUT/$g/Default.prf.retail" "$PARENT/$STAGE/$g/Profiles/Default.prf"
    FRESH+=("$STAGE/$g/autoexec.cfg" "$STAGE/$g/Profiles/Default.prf")
done
bash "$HERE/avp2cmds.sh" avp2 > "$PARENT/$STAGE/AVP2/avp2cmds.txt"
bash "$HERE/avp2cmds.sh" ph > "$PARENT/$STAGE/PrimalHunt/avp2cmds.txt"
CMDS=("$STAGE/AVP2/avp2cmds.txt" "$STAGE/PrimalHunt/avp2cmds.txt")

# what neither archive holds
EXCLUDE=()
for g in "${GAMES[@]}"; do
    EXCLUDE+=(--exclude="$NAME/$g/Save/*" --exclude="$NAME/$g/Profiles/*"
              --exclude="$NAME/$g/autoexec.cfg" --exclude="$NAME/$g/avp2cmds.txt")
done
EXCLUDE+=(--exclude='*.log' --exclude=run-cmds.txt --exclude='*.part')

rm -f "$PARENT/$NAME.tar" "$PARENT/$NAME.tar.gz"
if [[ $ARCHIVES == *" private "* ]]; then
    tar -C "$PARENT" -h -cf - "${EXCLUDE[@]}" \
        --transform="s|^$STAGE/|$NAME/|" \
        "$NAME" "${FRESH[@]}" "${CMDS[@]}" |
        gzip -6 > "$PKG.part"
    mv "$PKG.part" "$PKG"
    echo "   $PKG: $(du -h "$PKG" | cut -f1) (unpacks to $NAME/)"
fi
[[ $ARCHIVES == *" public "* ]] || exit 0

# The public archive: the install scripts beside the rest, the retail files
# left out.
PUB="$PARENT/$NAME-public.tar.gz"
rc=0
python3 "$HERE/mkinstall.py" "$OUT" "$PARENT/$STAGE" || rc=$?
if [ $rc = 3 ] && [ -n "${AVP2_ARCHIVES:-}" ]; then
    echo "   $PUB: no disc images, but AVP2_ARCHIVES asks for it" >&2
    exit 1
elif [ $rc = 3 ]; then
    echo "   $PUB: skipped (no disc images)"
    rm -f "$PUB"
    exit 0
elif [ $rc != 0 ]; then
    exit $rc
fi
sed "s|^|$NAME/|" "$PARENT/$STAGE/disc-files.txt" > "$PARENT/$STAGE/exclude.txt"
tar -C "$PARENT" -h -cf - "${EXCLUDE[@]}" \
    --anchored --no-wildcards --exclude-from="$PARENT/$STAGE/exclude.txt" \
    --transform="s|^$STAGE/|$NAME/|" \
    "$NAME" "${CMDS[@]}" "$STAGE/linux-install.sh" "$STAGE/windows-install.bat" \
    "$STAGE/mac-install.command" |
    gzip -6 > "$PUB.part"
mv "$PUB.part" "$PUB"
echo "   $PUB: $(du -h "$PUB" | cut -f1) (unpacks to $NAME/; its data from the discs)"
