#!/usr/bin/env bash
# Prove that include/ describes the same ABI as Kunlunxin's own headers.
#
# include/ is re-declared rather than imported (LICENSES.md), so every
# structure layout and every ioctl number in it has to match the vendor's
# exactly, or this driver and the vendor runtime will disagree about the
# bytes they pass through ioctl. This compiles the same probe twice -- once
# against the vendor headers, once against include/ -- and compares what
# the compiler made of each: field offsets from debug info, constants by
# value.
#
# The vendor headers are deliberately not in this repository. Point
# VENDOR_INCLUDE at the include/ directory of an extracted
# kunlun-module-dkms-4.33.0 (from the XRE 4.33.0.1 tarball) to run the
# comparison. With no path given the script says so and exits 0: a tree
# without the tarball cannot run it and does not need to.
set -uo pipefail
root="$(cd "$(dirname "$0")" && pwd)"
mod="$root/kunlun_module"
kdir="${KERNELDIR:-/lib/modules/$(uname -r)/build}"
work="${ABI_WORK:-$(mktemp -d)}"
[ -n "${ABI_WORK:-}" ] || trap 'rm -rf "$work"' EXIT

if [ -z "${VENDOR_INCLUDE:-}" ]; then
    echo "SKIP: set VENDOR_INCLUDE to an extracted vendor kunlun-module"
    echo "      include/ directory to compare against."
    exit 0
fi
[ -d "$VENDOR_INCLUDE/xpu" ] || { echo "FAIL: not a vendor include dir: $VENDOR_INCLUDE"; exit 1; }

headers=(xpu/defs.h xpu/version.h xpurt_priv/ioctl.h xpurt_priv/defs_private.h
         xpurt_priv/defs_private_kl1.h xpurt_priv/ioctl_kl1.h
         xpurt_priv/imgproc_defs.h xpurt_priv/ioctl_img_proc_kl2.h
         xpurt_priv/ioctl_vdec_kl2.h xpurt_priv/ioctl_venc_kl2.h
         xpurt_priv/vdec_cwl_defs.h xpurt_priv/vdec_dwl_defs.h
         xpurt_priv/venc_ewl_defs.h)

includes() { for h in "${headers[@]}"; do echo "#include \"$h\""; done; }

# The lists are generated from the headers under test, so they cannot drift
# from them. Tagged aggregates keep their tag; typedef'd ones are named bare.
awk '
  /^(struct|union|enum) [A-Za-z_][A-Za-z_0-9]* \{/ { print $1 " " $2; next }
  /^\} [A-Za-z_][A-Za-z_0-9]*;/ { n=$2; sub(/;$/,"",n); print n }
' "$root/include/kl_abi_types.h" | sort -u > "$work/typedecls"

grep -rhoE '^#define[[:space:]]+[A-Za-z_][A-Za-z_0-9]*[[:space:]]+[^[:space:]]' "$root/include" \
    | awk '{print $2}' | sort -u > "$work/macrolist"

echo "checking $(wc -l < "$work/typedecls") types and $(wc -l < "$work/macrolist") constants"

build() {  # $1 include dir, $2 tag, $3 probe body file
    local inc="$1" tag="$2" body="$3"
    local d="$work/$tag"   # separate: bash expands a multi-assignment
                           # local's words before assigning any of them
    mkdir -p "$d"
    { echo '#include <linux/module.h>'; includes; cat "$body"
      echo 'MODULE_LICENSE("GPL");'; } > "$d/probe.c"
    cat > "$d/Makefile" <<MK
obj-m += probe.o
ccflags-y += -I$inc -I$mod -I$mod/common/inc -DPLATFORM_KUNLUN -DENABLE_CODEC -Wno-error
MK
    make -C "$kdir" M="$d" KCFLAGS="-g -gdwarf-4" modules > "$d/log" 2>&1
}

# ---- layouts ------------------------------------------------------------
i=0
: > "$work/types.body"
while IFS= read -r t; do
    echo "volatile $t probe_$i;" >> "$work/types.body"; i=$((i+1))
done < "$work/typedecls"

types="$(sed 's/^\(struct\|union\|enum\) //' "$work/typedecls" | sort -u | tr '\n' ',' | sed 's/,$//')"
fail=0
echo
echo "== structure layouts, offsets included =="
for tag in vendor repo; do
    inc="$VENDOR_INCLUDE"; [ "$tag" = repo ] && inc="$root/include"
    if ! build "$inc" "t-$tag" "$work/types.body"; then
        echo "FAIL  type probe did not build against $inc"
        grep -m5 "error:" "$work/t-$tag/log"; fail=1; continue
    fi
    pahole -C "$types" --compile "$work/t-$tag/probe.o" 2>/dev/null \
        | grep -v '^typedef [a-z_ ]* [a-z_0-9]*;$' > "$work/$tag.types"
done
if [ "$fail" -eq 0 ] && diff -u "$work/vendor.types" "$work/repo.types" > "$work/types.diff"; then
    echo "PASS  $(grep -cE '^(struct|union|enum|typedef)' "$work/repo.types") definitions byte-identical"
elif [ "$fail" -eq 0 ]; then
    echo "FAIL  layouts differ:"; head -40 "$work/types.diff"; fail=1
fi

# ---- constants ----------------------------------------------------------
echo
echo "== ioctl numbers and constants, by value =="
emit_macros() {
    { echo 'const long probe_macro_values[] = {'
      while IFS= read -r m; do echo "	(long)($m),"; done < "$work/macrolist"
      echo '};'; } > "$work/macros.body"
}
# Drop anything that is not an integer constant expression in both sets
# (function-like macros, string literals); the rest are compared by value.
for round in 1 2 3 4 5 6 7 8; do
    emit_macros
    bad=0
    for tag in vendor repo; do
        inc="$VENDOR_INCLUDE"; [ "$tag" = repo ] && inc="$root/include"
        build "$inc" "m-$tag" "$work/macros.body" && continue
        bad=1
        offset=$(( 2 + ${#headers[@]} ))
        for ln in $(grep -oE 'probe\.c:[0-9]+:[0-9]+: error' "$work/m-$tag/log" \
                    | cut -d: -f2 | sort -un); do
            idx=$(( ln - offset ))
            [ "$idx" -ge 1 ] && [ "$idx" -le "$(wc -l < "$work/macrolist")" ] \
                && sed -i "${idx}s/.*/__DROP__/" "$work/macrolist"
        done
        sed -i '/^__DROP__$/d' "$work/macrolist"
    done
    [ "$bad" -eq 0 ] && break
done
if [ -f "$work/m-vendor/probe.o" ] && [ -f "$work/m-repo/probe.o" ]; then
    for tag in vendor repo; do
        objdump -s -j .rodata "$work/m-$tag/probe.o" 2>/dev/null | tail -n +4 > "$work/$tag.rodata"
    done
    if diff -u "$work/vendor.rodata" "$work/repo.rodata" > "$work/rodata.diff"; then
        echo "PASS  $(wc -l < "$work/macrolist") constant(s) identical in value"
    else
        echo "FAIL  constant values differ:"; head -20 "$work/rodata.diff"; fail=1
    fi
else
    echo "FAIL  constant probe did not build in both trees"; fail=1
fi

echo
if [ "$fail" -eq 0 ]; then echo "ABI identical"; else echo "ABI MISMATCH"; fi
exit "$fail"
