#!/bin/bash

SIM="./sim1"
DIR="../files"

# --- s27 ---
readarray -t s27_inputs <<'EOF'
1110101
0001010
1010101
0110111
1010001
EOF

# --- s298f_2 ---
readarray -t s298f_inputs <<'EOF'
10101010101010101
01011110000000111
11111000001111000
11100001110001100
01111011110000000
EOF

# --- s344f_2 ---
readarray -t s344f_inputs <<'EOF'
101010101010101011111111
010111100000001110000000
111110000011110001111111
111000011100011000000000
011110111100000001111111
EOF

# --- s349f_2 ---
readarray -t s349f_inputs <<'EOF'
101010101010101011111111
010111100000001110000000
111110000011110001111111
111000011100011000000000
011110111100000001111111
EOF

# arrays
netlists=("s27.txt" "s298f_2.txt" "s344f_2.txt" "s349f_2.txt")
input_arrays=("s27_inputs" "s298f_inputs" "s344f_inputs" "s349f_inputs")

# cleanup first
make clean
make

# loop
for i in "${!netlists[@]}"; do
    net="${netlists[$i]}"
    arr="${input_arrays[$i]}"
    declare -n IN_ARR="$arr"

    echo "================netlist: ${DIR}/${net}================"

    for j in "${IN_ARR[@]}"; do
        output=$("$SIM" -f "${DIR}/${net}" -i "$j")
        echo "INPUT=$j OUTPUT=$output"
    done

    echo ""
done
