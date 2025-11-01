./sim2 -f ../files/s27.txt -i 1101101 > ../files/output_s27_1101101.txt
./sim2 -f ../files/s27.txt -i 0101001 > ../files/output_s27_0101001.txt
./sim2 -f ../files/s298f_2.txt -i 10101011110010101 > ../files/output_s298f_2_10101011110010101.txt
./sim2 -f ../files/s298f_2.txt -i 11101110101110111 > ../files/output_s298f_2_11101110101110111.txt
./sim2 -f ../files/s344f_2.txt -i 101010101010111101111111 > ../files/output_s344f_2_101010101010111101111111.txt
./sim2 -f ../files/s344f_2.txt -i 111010111010101010001100 > ../files/output_s344f_2_111010111010101010001100.txt
./sim2 -f ../files/s349f_2.txt -i 101000000010101011111111 > ../files/output_s349f_2_101000000010101011111111.txt
./sim2 -f ../files/s349f_2.txt -i 111111101010101010001111 > ../files/output_s349f_2_111111101010101010001111.txt
python3 test.py -f ../files/s27.txt -m 100 -s 1 -t 40 -o ../images/s27.png
python3 test.py -f ../files/s298f_2.txt -m 300 -s 3 -t 404 -o ../images/s298f_2.png
python3 test.py -f ../files/s344f_2.txt -m 200 -s 2 -t 380 -o ../images/s344f_2.png
python3 test.py -f ../files/s349f_2.txt -m 200 -s 2 -t 378 -o ../images/s349f_2.png
