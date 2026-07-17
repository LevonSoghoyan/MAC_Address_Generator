#!/usr/bin/env bash

set -euo pipefail

PROGRAM_NAME="$(basename "$0")"
BASE_URL="${MAC_SERVER_URL:-https://127.0.0.1}"

MODE=""
FORCE=false
EMAIL=""
SN=""
PN=""
COUNT=1

usage() {
    cat <<EOF
Usage:
  $PROGRAM_NAME --generate --email EMAIL --sn SN --pn PN [--count COUNT] [--force]
  $PROGRAM_NAME --show --email EMAIL
  $PROGRAM_NAME --help

Modes:
  -g, --generate       Generate MAC address(es).
  -s, --show           Show active MAC addresses belonging to EMAIL.

Options:
  -e, --email EMAIL    Operator/customer email address.
  -n, --sn SN          Device serial number.
  -p, --pn PN          Device part number.
  -c, --count COUNT    Number of MAC addresses to generate. Default: 1.
  -f, --force          Archive existing allocation(s) and generate new MAC(s).
  -u, --url URL        Server base URL.
                       Default: \$MAC_SERVER_URL or ${BASE_URL}
  -k, --insecure       Allow insecure HTTPS certificates.
  -h, --help           Show this help message.

Examples:
  $PROGRAM_NAME -g -e operator@company.com -n SN000123 -p TT314
  $PROGRAM_NAME -g -e operator@company.com -n SN000123 -p TT314 -c 4
  $PROGRAM_NAME -g -f -e operator@company.com -n SN000123 -p TT314 -c 2
  $PROGRAM_NAME -s -e operator@company.com
EOF
}

error() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

urlencode() {
    local value="${1-}"

    if command -v python3 >/dev/null 2>&1; then
        python3 - "$value" <<'PY'
import sys
from urllib.parse import quote
print(quote(sys.argv[1], safe=""))
PY
    else
        # Basic fallback for common values.
        local encoded=""
        local i char hex

        for ((i = 0; i < ${#value}; i++)); do
            char="${value:i:1}"
            case "$char" in
                [a-zA-Z0-9.~_-])
                    encoded+="$char"
                    ;;
                *)
                    printf -v hex '%%%02X' "'$char"
                    encoded+="$hex"
                    ;;
            esac
        done

        printf '%s\n' "$encoded"
    fi
}

INSECURE=false

request() {
    local url="$1"
    local response_file
    local http_code
    local curl_status=0

    response_file="$(mktemp)"
    trap 'rm -f "$response_file"' RETURN

    local curl_args=(
        --silent
        --show-error
        --location
        --connect-timeout 10
        --max-time 60
        --output "$response_file"
        --write-out '%{http_code}'
    )

    if [[ "$INSECURE" == true ]]; then
        curl_args+=(--insecure)
    fi

    http_code="$(curl "${curl_args[@]}" "$url")" || curl_status=$?

    if ((curl_status != 0)); then
        rm -f "$response_file"
        trap - RETURN
        error "curl failed with exit code ${curl_status}."
    fi

    cat "$response_file"
    rm -f "$response_file"
    trap - RETURN

    if [[ ! "$http_code" =~ ^2[0-9][0-9]$ ]]; then
        printf '\nHTTP request failed with status %s\n' "$http_code" >&2
        exit 1
    fi
}

while (($# > 0)); do
    case "$1" in
        -g|--generate)
            [[ -z "$MODE" ]] || error "Choose only one mode: --generate or --show."
            MODE="generate"
            shift
            ;;

        -s|--show)
            [[ -z "$MODE" ]] || error "Choose only one mode: --generate or --show."
            MODE="show"
            shift
            ;;

        -f|--force)
            FORCE=true
            shift
            ;;

        -e|--email)
            (($# >= 2)) || error "$1 requires a value."
            EMAIL="$2"
            shift 2
            ;;

        -n|--sn)
            (($# >= 2)) || error "$1 requires a value."
            SN="$2"
            shift 2
            ;;

        -p|--pn)
            (($# >= 2)) || error "$1 requires a value."
            PN="$2"
            shift 2
            ;;

        -c|--count)
            (($# >= 2)) || error "$1 requires a value."
            COUNT="$2"
            shift 2
            ;;

        -u|--url)
            (($# >= 2)) || error "$1 requires a value."
            BASE_URL="${2%/}"
            shift 2
            ;;

        -k|--insecure)
            INSECURE=true
            shift
            ;;

        -h|--help)
            usage
            exit 0
            ;;

        *)
            error "Unknown argument: $1. Use --help for usage."
            ;;
    esac
done

[[ -n "$MODE" ]] || error "Select --generate or --show."
[[ -n "$EMAIL" ]] || error "--email is required."

BASE_URL="${BASE_URL%/}"
ENCODED_EMAIL="$(urlencode "$EMAIL")"

case "$MODE" in
    generate)
        [[ -n "$SN" ]] || error "--sn is required in generate mode."
        [[ -n "$PN" ]] || error "--pn is required in generate mode."

        [[ "$COUNT" =~ ^[0-9]+$ ]] ||
            error "--count must be a positive integer."

        ((COUNT >= 1 && COUNT <= 50)) ||
            error "--count must be between 1 and 50."

        ENCODED_SN="$(urlencode "$SN")"
        ENCODED_PN="$(urlencode "$PN")"

        if [[ "$FORCE" == true ]]; then
            ENDPOINT="/force_gen"
            printf 'Force-generating %d MAC address(es) for SN=%s, PN=%s, email=%s\n' \
                "$COUNT" "$SN" "$PN" "$EMAIL" >&2
        else
            ENDPOINT="/gmac"
            printf 'Generating %d MAC address(es) for SN=%s, PN=%s, email=%s\n' \
                "$COUNT" "$SN" "$PN" "$EMAIL" >&2
        fi

        URL="${BASE_URL}${ENDPOINT}?email=${ENCODED_EMAIL}&sn=${ENCODED_SN}&pn=${ENCODED_PN}&count=${COUNT}"
        request "$URL"
        printf '\n'
        ;;

    show)
        [[ "$FORCE" == false ]] ||
            error "--force can only be used with --generate."

        [[ "$COUNT" == "1" ]] ||
            error "--count is only valid with --generate."

        [[ -z "$SN" ]] ||
            error "--sn is only valid with --generate."

        [[ -z "$PN" ]] ||
            error "--pn is only valid with --generate."

        printf 'Showing active MAC addresses for email=%s\n' "$EMAIL" >&2

        URL="${BASE_URL}/find_active?email=${ENCODED_EMAIL}&sn=&pn="
        request "$URL"
        printf '\n'
        ;;
esac
