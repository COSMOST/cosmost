cat apps1.csv | while read app
    do
        ./eval.sh $app
    done
