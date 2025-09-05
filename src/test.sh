#!/bin/bash

SIM="./sim1"
DIR="../files"

# --- s27 ---
readarray -t s27_inputs <<'EOF'
1110101
0001010
EOF

readarray -t s27_expected <<'EOF'
1001
0100
EOF

# --- s298f_2 ---
readarray -t s298f_inputs <<'EOF'
10101010101010101
01011110000000111
EOF

readarray -t s298f_expected <<'EOF'
00000010101000111000
00000000011000001000
EOF

# --- s344f_2 ---
readarray -t s344f_inputs <<'EOF'
101010101010101011111111
010111100000001110000000
EOF

readarray -t s344f_expected <<'EOF'
10101010101010101010101101
00011110000000100001111100
EOF

# --- s349f_2 ---
readarray -t s349f_inputs <<'EOF'
101010101010101011111111
010111100000001110000000
EOF

readarray -t s349f_expected <<'EOF'
10101010101010101101010101
00011110000000101011110000
EOF

# arrays
netlists=("s27.txt" "s298f_2.txt" "s344f_2.txt" "s349f_2.txt")
input_arrays=("s27_inputs" "s298f_inputs" "s344f_inputs" "s349f_inputs")
expected_arrays=("s27_expected" "s298f_expected" "s344f_expected" "s349f_expected")

run_suite() {
    local netlist="$1"
    local -n IN_ARR="$2"
    local -n EXP_ARR="$3"

    echo "===============netlist: ${DIR}/${netlist}==============="

    local pass=0 fail=0
    for i in "${!IN_ARR[@]}"; do
        local in="${IN_ARR[$i]}"
        local exp="${EXP_ARR[$i]}"

        out="$("$SIM" -f "${DIR}/${netlist}" -i "$in" 2>&1 | tail -n 1)"

        if [[ "$out" == "$exp" ]]; then
        echo "  [#$((i+1))] PASS  INPUT=$in  OUT=$out"
        ((pass++))
        else
        echo "  [#$((i+1))] FAIL  INPUT=$in"
        echo "    EXPECT: $exp"
        echo "    GOT   : $out"
        ((fail++))
        fi
    done

    echo "-- ${netlist} Summary: PASS=${pass}  FAIL=${fail} --"
    echo ""
    [[ $fail -eq 0 ]]
}

# cleanup first
make clean
make

# then test
overall_pass=true

for i in "${!netlists[@]}"; do
    net="${netlists[$i]}"
    arr="${input_arrays[$i]}"
    exp="${expected_arrays[$i]}"
    run_suite "$net" "$arr" "$exp" || overall_pass=false
done

echo
if $overall_pass; then
  echo "All suites PASS"
  exit 0
else
  echo "Some suites FAILED"
  exit 1
fi
