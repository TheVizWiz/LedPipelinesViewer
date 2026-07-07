#!/usr/bin/env bash
# Regenerate src/viewer/IndexHtml.h from web/index.html so the browser page is embedded in the binary (making it
# runnable from any working directory). Run this after editing web/index.html.
set -euo pipefail
cd "$(dirname "$0")/.."

out=src/viewer/IndexHtml.h
{
	echo '// AUTO-GENERATED from web/index.html by scripts/embed_web.sh. Do not edit by hand.'
	echo '#pragma once'
	echo 'namespace viewer {'
	echo 'inline const char *kIndexHtml = R"HTMLDELIM('
	cat web/index.html
	echo ')HTMLDELIM";'
	echo '}'
} > "$out"

echo "wrote $out"
