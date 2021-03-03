./testaro Testing/file.txt

export e_code=$?

if [ $e_code != 0 ]; then
    printf "Test FAIL %d\n" "$e_code"
    exit 2
else
    printf "Test OK %d\n" "$e_code" 

fi
exit 0